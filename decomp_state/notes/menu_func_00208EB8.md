# func_00208EB8 (vram 0x208EB8, file 0x109E38, 32 bytes) — MATCHED 2026-09-04

Menu callback: `if (D_0013D2AC) *(int*)0x15EEB0 = 3;`.

## Identity / context

Ghidra has no function at 0x208EB8 (the whole 0x208800-0x2095xx menu block is
unanalyzed there); read directly from the generated asm and the surrounding
original disassembly. One DATA xref (function-pointer table entry at
0x1A0484) — a registered menu callback, no args, return ignored.

The sibling functions in the same block (func_00208E68 / func_00208E90 /
func_00208ED8 / func_00208F00) are near-clones that test bit 0x40 of
0x15EEB4 and store 3 to 0x15EEB0:

```
lui v0,0x16 / lw v0,-4428(v0) / andi v0,v0,0x40 / bnez v0,end
li v0,3          ; beqz/bnez delay slot
lui at,0x16
sw v0,-4432(at)  ; 0x15EEB0
```

So `li; lui; sw` (value load in the branch delay slot) is the established
codegen of this family. D_0015EEB0/0x15EEB4 sit in the core.lit.lit4
zero-pad region; D_0013D2AC is an undefined sym (build/undefined_syms_auto.txt).

## Codegen findings (IMPORTANT)

Plain global store does NOT reproduce the family schedule:

- `extern "C" int D_0015EEB0; *(...)` — without a section attribute EGC
  emits GP-relative access for in-window addresses (0x15EEB0 is inside
  gp=0x166C00 ± 32K). Out-of-window symbols (D_0013D2AC) instead fail the
  link with `relocation truncated to fit: R_MIPS_GPREL16` + "small-data
  section exceeds 64KB".
- With `__attribute__((section(".data")))` on both globals, EGC emits
  absolute lui/sw, but schedules the STORE-ADDRESS `lui` into the beqz
  delay slot (`beqz; <lui>; li; sw; jr; nop`) — one slot out from the
  original (`beqz; <li>; lui; sw; jr; nop`).
- The constant-cast store `*(int*)0x15EEB0 = 3;` matches exactly:
  `li v0,3` lands in the beqz delay slot, `lui at,0x16` after it, and the
  `sw` uses `at` as base — byte-identical, same registers (v0/v1/at).
  The cast makes the store target a compile-time constant, which changes
  EGC's delay-slot scheduling (constant address load is no longer a
  symbol-based address materialization competing for the slot).

Standalone EGC -G8 -O2 tests (7 source variants) confirmed only the
constant-cast / pointer-deref form reproduces the original order; plain
global, array-index, global-struct-field, ternary and early-return forms
all keep the `lui`-first schedule. Project precedent for this idiom:
`endDisplay()` in code/game/movie/disp.cpp (`*(int*)0x1611E0 = 0;`).

## Source

Replaced the INCLUDE_ASM in code/game/menu.cpp (was line 289) with:

```cpp
extern "C" int D_0013D2AC __attribute__((section(".data")));

extern "C" void func_00208EB8(void) {
    if (D_0013D2AC) {
        *(int*)0x15EEB0 = 3;
    }
}
```

## Verification

- Built image slice file 0x109E38..0x109E58 (32 bytes):
  `1400023c acd2438c 03006010 03000224 1600013c b0ee22ac 0800e003 00000000`
  byte-identical to the original.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- decomp_status --count: 827 -> 826.
