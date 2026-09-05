# InitMobyClassDists (0x0020D1F0, file 0x10E170, 0x28 bytes)

## What it does

Fills the moby class-distance table at EE DMA address 0x70003A00 with the
32-bit word 0x40000000, 0x380 bytes (0xE0 words):
`FastMemSet((void*)0x70003A00, 0x40000000, 0x380);` — a single-call
wrapper. FastMemSet (code/_generated/game/fastfunc.s, extern "C" in
common.h) is a word-fill loop (sw a1,0(a0) with size/4 iterations).
Siblings StashMobyClassDists/RestoreMobyClassDists move the same 0x380
bytes between D_001B2E80 and 0x70003A00 via FastMemCopy.

## Codegen

0x10 frame; EGC schedules the three constant args around the sq:
`lui a0,0x7000` before sq, `lui a1,0x4000` after, `ori a0` after that,
`li a2,0x380` in the jal delay slot. A plain single statement reproduces
it exactly (no temp variables, no reordering needed). The (void*) cast on
the 0x70003A00 literal is needed for the void* parameter; 0x40000000 fits
the int pattern parameter as an unsigned constant.

## Verification

- Object: 10/10 words identical; R_MIPS_26 on FastMemSet at the jal
  resolves to original word 0x0C07E5FA (target 0x1F97E8).
- Full `make` + `cmp`: identical. Count 804 -> 803. decomp-verifier MATCH.
