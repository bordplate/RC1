# texResetCursor__Fv (vram 0x1E9128, file 0xEA0A8, 24 bytes) — MATCHED 2026-09-09

Resets the VRAM/GPU-resource bump allocator to its base at level start. Loads the
pool base `D_0015EE8C` into the allocation cursor `D_0015EE74`, and zeroes the
allocation counter `D_0015EF20`:

```
1e9128:  lui   v0, %hi(D_0015EE8C)     ; v0 = base (0x2C0000 at init)
1e912c:  lw    v0, %lo(D_0015EE8C)(v0)
1e9130:  lui   at, %hi(D_0015EF20)
1e9134:  sw    zero, %lo(D_0015EF20)(at)  ; counter = 0
1e9138:  jr    ra
1e913c:  sw    v0, %lo(D_0015EE74)(at') ; cursor = base   [GPREL16: -0x7D8C(gp)]
```

Only the final store is gp-relative (`sw v0,-0x7D8C(gp)`); the two constant-address
stores use `lui at` / `sw zero` (cast-address idiom), and the base load uses
`lui v0`/`lw v0`. `D_0015EE74` is declared a plain `extern "C" int` (gp window),
the other two as constant-address casts.

## Identity / context

Part of the `game/actuator` segment. The allocator pool base `D_0015EE8C` is set to
`0x2C0000` (VRAM) by the init routine at 0x1EA830, which also seeds the cursor
`D_0015EE74` and a second cursor `D_0015EE78` to the same base, and sizes the pool
(`D_00160F0C = 0x100000`).

Consumer pattern (e.g. 0x203120 VU-program loader, 0x231878 texture/palette
allocator): read cursor `D_0015EE74`, allocate a chunk (cursor += 0x400 / 0x1000 /
0x8000), write back, and build GS base-pointer words (GSB0/GSBP0, 0x19304000 /
0x580000000) from `cursor >> 8`. So `D_0015EE74` = next-free cursor, `D_0015EE8C`
= pool base, `D_0015EF20` = allocation counter (written only by this function in the
boot ELF).

Sole caller: `startlevel__Fv` (0x1E9658, bmain.cpp) at 0x1E9688, early in level
init, right after 0x201650 and before zeroing the 0x41B0-byte table at 0x161280.

## Split / codegen findings

The original `func_001E9120` was a 0x20-byte block (0x1E9120..0x1E913F) that spimdis
treated as one function: a 4-byte dead fragment (`addiu sp,sp,0x20; nop` at 0x1E9120)
plus this 24-byte live function at 0x1E9128, with two trailing nops (0x1E9140,
0x1E9144) emitted after `endlabel`.

Adding `texResetCursor__Fv = 0x1e9128` to config/symbols.txt makes Splat split the
block into:
- `func_001E9120` — 4-byte dead-tail orphan (kept as INCLUDE_ASM; supplies the 2 words).
- `texResetCursor__Fv` — the 24-byte live function (now C).

EGC matches with a **void** return and direct stores:
```cpp
extern "C" int D_0015EE74;
void texResetCursor(void) {
    D_0015EE74 = *(int *)0x15EE8C;
    *(int *)0x15EF20 = 0;
}
```
An `int`-returning form (with or without `return D_0015EE74`) does NOT match: EGC
then loads the base into v1 and emits a dead-tail store. The void form is correct —
the v0 value at `jr ra` is a leftover, not a return value (the caller ignores it).

**Trailing-gap nops:** the two nops at 0x1E9140–0x1E9147 were emitted as trailing
bytes of the old `func_001E9120` INCLUDE_ASM. Once the live part becomes C, they are
lost and the object shrinks 8 bytes, shifting every downstream object (actuator.o
0x468 -> 0x460) and breaking cross-section relocations (startlevel__Fv moved
0x1E9658 -> 0x1E9650, visible as a constant diff in 989snd `main`). Fix: emit them
explicitly with file-scope `asm("nop"); asm("nop");` after the C function (same
convention as code/game/stash.cpp). This restores actuator.o to 0x468 and full
parity.

## Source

code/game/actuator.cpp:
```cpp
INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", func_001E9120);

extern "C" int D_0015EE74;

void texResetCursor(void) {
    D_0015EE74 = *(int *)0x15EE8C;
    *(int *)0x15EF20 = 0;
}

asm("nop");
asm("nop");

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", func_001E9148);
```
Symbol `texResetCursor__Fv = 0x1e9128;` added to config/symbols.txt (before the
bloaders block).

## Verification

- Standalone probe (v2, void + direct store): 24/24 bytes match.
- actuator.o .text = 0x468; object region 0x420–0x467 byte-identical to original
  0x1E9120–0x1E9167 except the pending R_MIPS_GPREL16 addend on D_0015EE74
  (resolves to 0x8274 = -0x7D8C, matching the original `sw v0,-0x7D8C(gp)`).
- decomp-verifier: MATCHED; clean `make clean && make split && make -j2` +
  `cmp build/boot_elf.elf assets/boot_elf.elf` = PARITY_OK.
- Remaining actuator.cpp nonmatches: func_001E8D00 (4B), actuator_CalcPower (0x414),
  func_001E9120 (4B dead tail), func_001E9148 (0x1C dead tail) — all INCLUDE_ASM.
