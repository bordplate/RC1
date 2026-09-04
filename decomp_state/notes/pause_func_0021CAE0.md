# func_0021CAE0 (vram 0x21CAE0, file 0x11DA60, 32 bytes) — MATCHED 2026-09-04

One-line pause-menu callback: `func_00225AC0(1); return 0;`.

## Identity / context

func_00225AC0 (vram 0x225AC0, still INCLUDE_ASM) is the pause-screen sound
slot allocator: it fills the 5-slot table at DAT_001d60b8 with handle pairs
(starting at D_001D5CF8, step 0x11800 for the first 2 slots, then
D_001D5CFB... step 0x4F000, zero-fill remainder), parameter = number of
"active" slots (0 or 1). Ghidra decompiles func_0021CAE0 as
`FUN_00225ac0(1); return 0;`.

All 8 xrefs are DATA (function-pointer table entries at 0x1CF990, 0x1D0490,
0x1D08E0, 0x1D27B0, 0x1D2AC8, 0x1D2CD0, 0x1D49D0, 0x1D4B48) — the function
is a registered callback, called with no arguments from the menu dispatch
code, which consumes the int return.

## Codegen findings

EGC -O2 for a zero-arg function whose only statements are a call with
literal arg 1 and `return 0`:

```
addiu sp,sp,-0x10
sq    ra,0(sp)
jal   func_00225AC0
addiu a0,0,1      ; delay slot (word 0x01000424)
lq    ra,0(sp)
daddu v0,0,0      ; return 0, scheduled between lq and jr
jr    ra
addiu sp,sp,0x10  ; delay slot
```

Extension of the stash_func_00232CE0 single-call-wrapper pattern: the
literal-arg load lands in the `jal` delay slot (callee prototype MUST take
a parameter), and the constant return `daddu $v0,$0,$0` is hoisted into the
slot between `lq ra` and `jr ra` (the `jr` delay slot is taken by the
standard `addiu sp` restore — cf. func_0021CAC8, where the same `return 0`
`daddu` lands in the `jr` delay slot when there is no frame).

## Source

Replaced the INCLUDE_ASM in code/game/pause.cpp (was line 82) with:

```cpp
extern "C" void func_00225AC0(int param_1);

extern "C" int func_0021CAE0(void) {
    func_00225AC0(1);
    return 0;
}
```

func_00225AC0 stays INCLUDE_ASM further down in the same file; the
prototype above is what lets the call emit its argument setup.

## Verification

- Built image slice file 0x11DA60..0x11DA80 (32 bytes):
  `f0ffbd27 0000bf7f b096080c 01000424 0000bf7b 2d100000 0800e003 1000bd27`
  byte-identical to the original.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- decomp_status --count: 828 -> 827.
