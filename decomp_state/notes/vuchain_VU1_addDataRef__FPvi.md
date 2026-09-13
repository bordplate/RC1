# VU1_addDataRef__FPvi (0x233830, 0x4C) — MATCHED 2026-09-13

Bump allocator for the VU1 command chain. `vu1ChainHead` (0x160F00, renamed
from D_00160F00) holds the bump pointer V. Writes a 16-byte data-reference
record `[0x30000000|tag, dataRef, 0, 0]` at [V..V+0xC], then advances the head
to V+4 words. The draw subsystem appends packets through the same head (see
PutDrawBufferSmall/Large), and VU1_initChain initializes it.

## Solution

`code/game/vuchain.cpp`, with `vuchain.o` built using
`-mno-split-addresses` (Makefile PRIVATE_COMPILE_FLAGS):

```cpp
extern volatile u32* volatile vu1ChainHead __attribute__((section(".data")));
extern volatile u32* vu1ChainHeadStore;   // linker alias, same 0x160F00

void VU1_addDataRef(void* dataRef, s32 tag) {
    vu1ChainHead[0] = VU1_DATA_REF_TAG | tag;   // 0x30000000
    vu1ChainHead[1] = (u32)dataRef;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = 0;
    vu1ChainHeadStore = vu1ChainHead + 4;
}
```

Two declaration kinds at one address are required, verified by probe:

1. Body loads/stores need a VOLATILE `.data` pointer. A plain named `.data`
   symbol lets EGC common-subexpression-eliminate the four later head loads
   into the first (60-byte output); constant-address casts hoist the constant
   into a shared base register (single-instr loads); plain in-window externs
   give one-instr GPREL loads (60-byte output). A volatile pointer (single or
   double) + `.data` + `-mno-split-addresses` emits the five separate
   self-based `lui r; lw r` loads in the original register pattern
   (v1, v0, v1, a0, v0) at the project-default -O2; the committed form is
   double-volatile (probeHD, byte-verified).
2. The final head update must be a GPREL store (`sw v0, -0x5D00(s0)` in the
   `jr ra` delay slot). Storing through the `.data` name emits an absolute
   `lui at; sw r,0(at)` pair instead, so a plain same-address alias
   (`vu1ChainHeadStore` in config/linker_aliases.ld; symbols.txt rejects
   duplicate VRAM addresses) is used for that one store.

## Dead tail func_00233880 (0x233880, 4 bytes)

The original follows the matched function with an unreachable
`sw v0, -0x3FE0(s0)` (writes the bumped value to vu1ChainTail, 0x162C20) —
EGC's dead-store tail. No source form regenerates it together with the
matched 76-byte body: return-based forms (`unsigned int* ret` with two stores
of a local) DO fire the dead store but shift body allocation (L4 v0 vs a0,
`move v0,v1` for the return); the void read-back form leaves the tail store in
the jr delay slot (80 bytes). The orphan INCLUDE_ASM is retained.

## History

2026-09-06: first blocked (self-based load pattern believed unregeneratable;
that retest did match the first 68 bytes with -mno-split-addresses + named
symbol but the final store was absolute, not GPREL). 2026-09-13: matched by
adding double volatile (defeats pointer CSE) and the plain GPREL store alias.
