# func_00208E68 (vram 0x208E68, file 0x109DE8, 36 bytes) — MATCHED 2026-09-04

2026-09-11 correction: the four callbacks now use the semantic
`menuPostFlags` and `menuPostCallbackIndex` symbols. Exact function-boundary
TUs give these callbacks `-mno-split-addresses`, which reproduces the original
symbolic load and store. The constant-cast findings below apply under the
previous flags and explain why the local compiler flag is required.

Menu callback: `if (*(int*)0x15EEB4 & 0x40) return; *(int*)0x15EEB0 = 3;`.

## Identity / context

Ghidra has no functions in this menu block; read from the generated asm. One
DATA xref (function-pointer table entry at 0x1A047C, same callback table as
func_00208EB8 at 0x1A0484) — a registered no-arg menu callback.

Part of a clone family: func_00208E90 / func_00208ED8 / func_00208F00 are
byte-identical (same 9 instructions, only the bnez target offset differs),
all test bit 0x40 of 0x15EEB4 and store 3 to 0x15EEB0.

## Codegen findings

Extends the func_00208EB8 finding (see menu_func_00208EB8.md): the
constant-cast idiom is required on BOTH the load and the store to reproduce
the original:

- `if (D_0015EEB4 & 0x40) { return; } *(int*)0x15EEB0 = 3;` (global load,
  cast store) compiles the load with base register $v1 (`lui v1; lw v0,
  off(v1)`), but the original reuses $v0 for base and value
  (`lui v0,0x16; lw v0,-4428(v0)`).
- `if (*(int*)0x15EEB4 & 0x40) { return; } *(int*)0x15EEB0 = 3;`
  (constant-cast load AND store) matches byte-for-byte, including the
  `li v0,3` in the bnez delay slot, `lui at,0x16` after it, and `at` as the
  store base. Standalone EGC tests of 5 other source forms
  (explicit `!= 0`, braceless return, temp variable, goto) all kept the
  v1-base schedule.

## Source

Replaced the INCLUDE_ASM in code/game/menu.cpp (was line 285) with:

```cpp
extern "C" void func_00208E68(void) {
    if (*(int*)0x15EEB4 & 0x40) {
        return;
    }
    *(int*)0x15EEB0 = 3;
}
```

## Verification

- Built image slice file 0x109DE8..0x109E0C (36 bytes):
  `1600023c b4ee428c 40004230 03004014 03000224 1600013c b0ee22ac 0800e003 00000000`
  byte-identical to the original.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- decomp_status --count: 826 -> 825.

The three clone siblings (func_00208E90 / func_00208ED8 / func_00208F00)
were matched the same day with the identical source — each is byte-identical
in the object (only the bnez target differs, same relative distance), each
slice verified and full parity passing. The four-clone family is complete.
