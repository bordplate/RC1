# func_0022E188 (vram 0x22E188, file 0x12F108, 28 bytes) — matched 2026-09-05

## Semantics

Space-transition setup. Stores the requested space id and raises two
pending flags consumed by the transition code:

- `D_0015F600` = param (space id; `DoSpaceTransition` tests `< 8` right
  next to `GameMode`@0x15F604 / `worldUpdateTime`@0x15F60C)
- `D_0015F5B0` = 1 (flag read by `Transition_DoTransition__Fv`)
- `D_0015F618` = 1 (pending flag; read first by func_00230EE8, cleared by
  func_00230F60 at level init)

`li v0,1` is just the constant store value; the function is VOID and all
six callers ignore v0 (verified: 0x2222E8, 0x2233C8, 0x223790, 0x223998,
0x1FD53C, 0x1FD5CC pass 0 or a loaded global, never use the return).

## Original body (file offset 0x12F108, vram 0x22E188, 7 words)

```
24020001  li    v0, 1
3c010016  lui   at, 0x16
ac24f600  sw    a0, -2560(at)      # 0x15F600 = param
3c010016  lui   at, 0x16
ac22f618  sw    v0, -2536(at)      # 0x15F618 = 1
03e00008  jr    ra
af8289b0  sw    v0, -30288(gp)     # 0x15F5B0 = 1 (delay slot)
```

## Replacement (code/game/space.cpp)

```cpp
extern "C" int D_0015F5B0;

extern "C" void func_0022E188(int param_1) {
    *(int*)0x15F600 = param_1;
    D_0015F5B0 = 1;
    *(int*)0x15F618 = 1;
}
```

## Codegen findings (IMPORTANT)

1. **Return type drives the constant register.** For an `int`-returning
   function EGC reserves v0 for the return value and materializes body
   constant stores into v1 — two separate `li` instructions (one per use
   site), no hoisting. The original has ONE `li v0,1` at the top used by
   both constant stores. That only happens for a VOID function, where v0
   is free for the body constant. All 8 `int`-returning source forms
   (inline literals, `int x = 1` at top/middle, chained assignment
   `a = b = 1`, `return (b = (a = 1))`) compile to the same 8-instruction
   2-`li` shape; only the void form collapses to the original 7 words.
   (Standalone EGC -G8 -O2 matrix, see attempts below.)
2. `$at` base on the absolute stores confirms constant-cast accesses
   (`*(int*)0x15F600`); the gp-relative third store confirms a plain
   `extern "C" int` (no section attribute, in gp window).
3. **Store scheduling with two cast stores + one gp store:** source order
   `cast600; gp5B0; cast618` reproduces the original machine order
   `cast600; cast618; jr; <gp5B0 in delay slot>` with the `li v0,1`
   hoisted to the top. The naive order `cast600; cast618; gp5B0` instead
   puts the gp store second (right after the first cast store) — wrong.

## Verification (mechanical)

- objdump of build/code/game/space.o `func_0022E188`: 8 instructions
  (7 + trailing nop), identical encodings to the original; GPREL16 reloc
  on D_0015F5B0 fills -0x7650.
- Raw 28-byte slice at file offset 0x12F108 built == original
  (`01000224 1600013c 00f624ac 1600013c 18f622ac 0800e003 b08982af`).
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- decomp_status --count: 813 -> 812.
