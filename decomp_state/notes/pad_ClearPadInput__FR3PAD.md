# ClearPadInput__FR3PAD (code/game/pad.cpp) — MATCHED 2026-10-01

100 bytes (0x64) at vram `0x002172C0` (file offset `0x118240`). Default
flags (`-G8 -O2 -ffast-math -fno-exceptions -snas`), no pins, no private
TU flags.

## What it does

Clears the per-frame controller state in `PAD& self` (`padState`,
0x13C940):

- zeroes `field_1A0`, `pressedButtons` (0x1A4), `field_1A8`, `field_1B0`,
  `field_1B4`, `field_1B8`, `field_1C0`, `field_1C4`, `field_1C8`,
  `field_1D8`;
- sets `field_1D0 = 1` and `field_1D4 = 1` (both share one hoisted
  `li v0,1`);
- zeroes `analog[16]` (0x100) and `hudAnalog[16]` (0x140) in one loop.

Called from `UpdatePad__FR3PAD` (0x2170C8, still INCLUDE_ASM) on both the
"pad state changed" and "no change / clear" paths, so it runs every frame.

## Replacement (code/game/pad.cpp)

```cpp
void ClearPadInput(PAD& self) {
    self.field_1B0 = 0;
    self.field_1A0 = 0;
    self.pressedButtons = 0;
    self.field_1A8 = 0;
    self.field_1D0 = 1;
    self.field_1D4 = 1;
    self.field_1B4 = 0;
    self.field_1B8 = 0;
    self.field_1C0 = 0;
    self.field_1C4 = 0;
    self.field_1C8 = 0;
    self.field_1D8 = 0;
    for (int i = 0; i < 16; i++) {
        self.analog[i] = 0.0f;
        self.hudAnalog[i] = 0.0f;
    }
}
```

`PAD` (code/include/pad_state.h) was extended from 0x1A8 to 0x1E0: the two
16-float arrays at 0x100/0x140 and the packed u32 scalar region
0x1A0..0x1DC. Field names `field_XXX` are offset-based per STYLEGUIDE (no
confirmed semantics yet beyond `pressedButtons`); the arrays are named after
the equivalent `analog[]`/`hudAnalog[]` members in the Deadlocked PAD struct
(reference/dl/game_dl/pad.h) — ProcessPadInput fills 0x100 from the raw pad
bytes with float math and then copies the 16 floats verbatim to 0x140
(copy loop at 0x217520: `lwc1 0x100..; swc1 0x140..`, 16 iterations).

## Codegen finding: EGC constant-store hoisting (the blocker here)

The two `= 1` stores are NOT emitted in source order. EGC 2.95.2 hoists the
LATER-in-source of the two constant stores to position 2 (immediately after
the first store of the function), and keeps every other store in its
source-relative position. Verified with standalone probes (10 source
permutations, all consistent):

| source (two 1s marked) | EGC output |
|---|---|
| 1B0, **1D4**, 1A0, 1A4, 1A8, **1D0**, 1B4.. | 1B0, 1D0, 1D4, 1A0, 1A4, 1A8, 1B4.. |
| 1A0, 1A4, 1A8, **1D0**, 1B4.., 1D8, **1D4**, 1B0 | 1A0, 1D4, 1A4, 1A8, 1D0, 1B4.., 1D8, 1B0 |
| 1A0, 1A4, 1A8, 1B0.., 1C8, **1D0**, **1D4**, 1D8 | 1A0, 1D4, 1A4, 1A8, 1B0.., 1C8, 1D0, 1D8 |

Consequence: to get the original order `1B0, 1D4, 1A0, 1A4, 1A8, 1D0,
1B4, ...`, the source must put **1D4 after 1D0** (1D4 is the hoisted one).
The source order above (1D0=1 before 1D4=1) is what matches. Writing them
in the original's apparent output order (1D4 before 1D0) instead compiles
to `1B0, 1D0, 1D4, 1A0, ...` (a 2-word swap) — do not "fix" the source
order to look like the assembly.

`-fno-schedule-insns` does not change this: the hoisting happens before the
scheduler (both scheduler flags tested), so no private flag is needed.

## Loop shape

`for (int i = 0; i < 16; i++) { analog[i] = 0.0f; hudAnalog[i] = 0.0f; }`
compiles to the original exactly:

```
addiu v1, a0, 0x140      # base = &hudAnalog (second array)
li    a1, 15             # 16 iterations via countdown + bgez
loop:
sw    zero, -0x40(v1)    # analog[i]
addiu a1, a1, -1
sw    zero, 0(v1)        # hudAnalog[i]
nop
nop
bgez  a1, loop
  addiu v1, v1, 4
```

EGC picks the SECOND array (larger offset) as the loop base and addresses
the first at `-0x40(base)`. Assigning `0.0f` to the f32 fields emits the
same `sw zero` as an int store (constant 0.0f has bit pattern 0), so `f32`
is the correct type. `li 15` + `bgez` (not `li 16` + `bgtz`) is EGC's
idiom for `i < 16`.

## Verification

- Raw 100-byte object comparison of `ClearPadInput__FR3PAD` in
  build/code/game/pad.o vs the generated nonmatching .s: 0 byte diffs.
- `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf`: byte-for-byte match. pad_state.h is a top-level
  dependency, so all including TUs (pad, stream, bmain, draw_occl) were
  rebuilt in the clean build.
- Lombyte (reference/Lombyte/src/textbin/fun_002172c0.c) independently
  decompiled the same function (Ghidra pseudo-C); its statement order
  differs but the semantics agree.
