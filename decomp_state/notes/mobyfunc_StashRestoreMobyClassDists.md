# StashMobyClassDists (0x0020D218) / RestoreMobyClassDists (0x0020D248)

## What they do

Move the 0x380-byte moby class-distance table between the RAM stash
D_001B2E80 (.data) and the live EE DMA buffer at 0x70003A00 (the same
buffer InitMobyClassDists fills):

```cpp
extern char D_001B2E80[] __attribute__((section(".data")));

void StashMobyClassDists() {
    FastMemCopy(D_001B2E80, (void*)0x70003A00, 0x380);
}

void RestoreMobyClassDists() {
    FastMemCopy((void*)0x70003A00, D_001B2E80, 0x380);
}
```

FastMemCopy(dst, src, size) is the 128-bit-step copy loop in
code/_generated/game/fastfunc.s (lq from src, sq to dst-0x10, size/16
iterations); declared in common.h next to FastMemSet.

## Codegen

0x10 frame; EGC interleaves the two constant address materializations
around the sq (dst hi before sq, src hi + dst lo + src lo/ori after),
`li a2,0x380` in the jal delay slot. The (void*) cast on 0x70003A00
keeps the constant in the void* param; D_001B2E80 is outside the gp
window so the declaration with section(".data") yields the plain
HI16/LO16 pair the original uses.

## Verification

- Objects: 11/11 words identical for each function; R_MIPS_HI16/LO16 on
  D_001B2E80 resolve to 0x1B2E80, R_MIPS_26 on FastMemCopy to 0x1F98D0.
- Full `make` + `cmp`: identical. Count 803 -> 801 (both). decomp-verifier
  MATCH for both.
