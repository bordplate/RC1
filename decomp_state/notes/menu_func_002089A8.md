# func_002089A8 (vram 0x2089A8, file 0x109928, 36 bytes) — MATCHED 2026-09-04

2026-09-06: menu.cpp now uses `-fno-schedule-insns` to unblock the edge test.
The equivalent source order is E0, state, DC under that flag. Full boot cmp
passes; the bytes below are unchanged. The older permutation explanation
applies only to the previous default flags.

Menu callback over global struct at D_0013D290:

```cpp
*(int*)0x15EEB0 = 4;
D_0013D290.field_0xDC = -1;
D_0013D290.field_0xE0 = -1;
```

## Identity / context

Ghidra has no functions in this menu block; read from the generated asm.
One DATA xref (function-pointer table entry) — a registered no-arg menu
callback. Sets the 0x15EEB0 "state" to 4 and clears two words of the
D_0013D290 menu-data struct to -1.

D_0013D290 is an undefined sym (build/undefined_syms_auto.txt:54) at 0x13D290,
outside the gp window, so it needs `__attribute__((section(".data")))` to get
absolute lui/addiu addressing (a plain `extern` ref would fail the link with
R_MIPS_GPREL16 truncation). The struct is declared with the two touched fields
at their exact offsets (pad to 0xDC).

## Codegen findings

Three stores: two to the struct (sharing base $v0 and a single CSE'd value
`addiu $a0,$0,-1`) and one to 0x15EEB0 (value `addiu $v1,$0,4`, base $at via
the constant-cast idiom from menu_func_00208E68.md).

The three-store fixed permutation from AGENTS.md applies: emitted
`stmt3; stmt1; jr $ra; <delay slot: stmt2>`. To reproduce the original
(`sw 0xE0; sw 0x15EEB0; jr; <ds sw 0xDC>`), the source statement order is
(stmt1=0x15EEB0, stmt2=0xDC, stmt3=0xE0) — i.e. write them as
`0x15EEB0; 0xDC; 0xE0`. EGC CSEs the two -1 stores into one `addiu $a0,$0,-1`
used by both, and the lower-offset store (0xDC) lands in the jr delay slot.

## Source

Replaced the INCLUDE_ASM in code/game/menu.cpp (was line 261). Added the
shared `MenuData_0013D290` struct + global declaration above the function.

## Verification

- Built image slice file 0x109928..0x10994C (36 bytes):
  `1400023c ffff0424 90d24224 04000324 e00044ac 1600013c b0ee23ac 0800e003 dc0044ac`
  byte-identical to the original.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- decomp_status --count: 821 -> 820.

Sister func_002088A8 (same D_0013D290 struct, offsets 0x1C/0xBC/0xF4) is still
INCLUDE_ASM — it hits a scheduler divergence (value/base/lw ordering) that does
not reproduce with any statement order tried; see attempts record.
