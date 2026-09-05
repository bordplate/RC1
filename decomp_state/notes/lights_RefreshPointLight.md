# RefreshPointLight (code/game/lights.cpp) — matched 2026-09-05

## Semantics

Rebuilds one dynamic point-light slot. The four lights functions live in
`code/game/lights.cpp` and operate on the level's light tables
(`D_0019C1C0`/`D_0019C3C0` families, 0x30-byte light records indexed by the
slot id). `RefreshPointLight(idx)`:

```
DetachPointLight(idx);   // clear the slot's old link-list entries
CreatePointLight(idx);   // rebuild the slot from the current camera/rot data
```

Both callees are `void` and take the slot index in `$a0`. The wrapper returns
`$v0` left by `CreatePointLight` (the caller `UpdateAllPointLights` at
0x00201A28 discards it, so it is semantically `void`).

Mangling split in this file (read from the glabels):

- `UpdateAllPointLights`, `CreatePointLight`, `RefreshPointLight` — unmangled
  (C linkage).
- `DetachPointLight__Fi` — C++-mangled free function `DetachPointLight(int)`.

So `RefreshPointLight` is an unmangled function that calls one mangled and one
unmangled callee. That mix is the interesting part (see Codegen findings #1).

## Original body (file offset 0x102ED8, vram 0x00201F58, 11 words)

```
27bdffe0  addiu $sp,$sp,-0x20
7fb00000  sq    $16,0x0($sp)
7fbf0010  sq    $ra,0x10($sp)
<jal>       jal   DetachPointLight__Fi
2d808000    daddu $16,$a0,$0      # save idx across call 1 (delay slot)
<jal>       jal   CreatePointLight
2d200002    daddu $a0,$16,$0      # restore idx for call 2 (delay slot)
7bbf0010  lq    $ra,0x10($sp)
7bb00000  lq    $16,0x0($sp)
0800e003  jr    $ra
2000bd27    addiu $sp,$sp,0x20
```

## Replacement (lights.cpp)

```cpp
INCLUDE_ASM("code/_generated/nonmatchings/game/lights", CreatePointLight);

extern "C" void CreatePointLight(int idx);
void DetachPointLight(int idx);

void RefreshPointLight(int idx) asm("RefreshPointLight");
void RefreshPointLight(int idx) {
    DetachPointLight(idx);
    CreatePointLight(idx);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/lights", DetachPointLight__Fi);
```

- `CreatePointLight` is declared `extern "C"` so the call references the
  unmangled symbol the INCLUDE_ASM below defines.
- `DetachPointLight` is a plain C++ declaration so EGC mangles the call to
  `DetachPointLight__Fi` (cfront scheme, one `int` arg).
- `RefreshPointLight` keeps its unmangled name via an **asm label on the
  declaration** (not the definition — EGC's parser rejects `asm()` on the
  out-of-line definition, same as the zero-arg method case in AGENTS.md).
  The label persists to the adjacent definition.

## Codegen findings

1. **Unmangled name for a C++ free function that calls a mangled callee.**
   `extern "C"` cannot be used: inside an `extern "C"` body every call gets C
   linkage, so `DetachPointLight(idx)` would reference the unmangled
   `DetachPointLight` (wrong — the binary needs `DetachPointLight__Fi`).
   Instead declare the function in C++ and force the symbol name with
   `void RefreshPointLight(int idx) asm("RefreshPointLight");` on the
   PROTOTYPE, then define it normally. Verified: EGC 2.95.2/v2.73a rejects
   `asm()` on the definition (`parse error before '{'`) but accepts it on the
   declaration; the emitted symbol is `T RefreshPointLight` and the call
   relocs are `DetachPointLight__Fi` + `CreatePointLight`.
   This is the free-function analogue of the in-class `asm()` recipe in
   AGENTS.md (music_Unpause__Fv) and should be the go-to idiom whenever an
   unmangled binary symbol is a wrapper around mangled C++ callees.

2. **Two-call wrapper argument scheduling.** For
   `f(x); g(x);` where both take the same arg by value, EGC copies the arg
   into `$16` (s0) in the FIRST `jal`'s delay slot and copies it back to
   `$a0` in the SECOND `jal`'s delay slot, with a 0x20 frame saving only
   `$16`/`$ra` (sq $16,0 / sq $ra,0x10; lq $ra,0x10 / lq $16,0). The tail is a
   bare `jr $ra` with the `addiu sp,sp,0x20` in the delay slot — no `move` of
   `$v0`, so a `void` return reproduces it exactly (declaring `CreatePointLight`
   `int`-returning and using `return ...` compiles to the identical body here).
   Matched on the first try.

3. **Object layout preserved.** In lights.o the C-compiled `RefreshPointLight`
   is emitted at .text+0x530, exactly between the `CreatePointLight` asm
   block (+0x180) and the `DetachPointLight__Fi` asm block (+0x560), with the
   4-byte `.align 3` gap (0x55C..0x55F) before the next function — identical
   relative layout to the original (vram 0x1F58, 0x1F88). GCC/EGC emits the
   file-scope `__asm__` INCLUDE_ASM blocks and the C function in source order,
   so keeping the C definition between the two INCLUDE_ASM lines was enough.

## Verification (mechanical)

- objdump of build/code/game/lights.o `RefreshPointLight` (0x530..0x55B):
  11 instructions, identical sequence/registers to the original; `jal`
  relocs R_MIPS_26 to `DetachPointLight__Fi` (+0x560) and `CreatePointLight`
  (+0x180); `nm` shows `T RefreshPointLight` (no `.NON_MATCHING` alias).
- Raw 44-byte slice at file offset 0x102ED8: built == original
  (`e0ffbd27 0000b07f 1000bf7f e207080c 2d808000 ea06080c 2d200002 1000bf7b
   0000b07b 0800e003 2000bd27`).
- Full clean `make clean && make split && make -j2` +
  `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- `decomp_status.py --count` 814 -> 813.

## Follow-ups

- `CreatePointLight` (0x00201BA8, 0x3B0) and `DetachPointLight__Fi`
  (0x00201F88, 0x2E8) are the big siblings — both are the per-channel
  (3 link-lists of 0x1E/0x36-byte entries) light-index rewriters with the
  0xF000 "detached" tag logic; `UpdateAllPointLights` (0x00201A28, 0x180)
  drives all 8 slots and calls both `Create` and `Refresh`. Now that
  `RefreshPointLight` is C, those three only need their own bodies.
- The light record layout (0x19C1C0 base + idx*0x30, sub-list heads at
  +0x00/+0x08/+0x04 with counts at +0x02/+0x0A/+0x06) is now partially
  decoded from Detach; name the struct when Create/Detach are matched.
