#ifndef VIBUF_H
#define VIBUF_H

#include "types.h"

typedef struct ViBuf {
    u32 base;
    u32 tagBase;
    u32 blocks;
    u32 field_0x0c;
    u32 field_0x10;
    u32 field_0x14;
    u32 capacity;
    u32 dmac4ToMadr;
    u32 dmac4ToTadr;
    u32 dmac4ToQwc;
    u32 dmac4ToChcr;
    u32 dmac3FromMadr;
    u32 dmac3FromQwc;
    u32 dmac3FromChcr;
    u32 ipuBp;
    u32 ipuCtrl;
    u32 sema;
    u32 dmaFlag;
    u64 field_0x48;
    u32 tags;
    u32 tagCount;
    u32 tagHead;
    u32 tagIdx;
} ViBuf;

// A ViBuf FIFO block is 0x800 (2048) bytes; field_0x10 counts blocks and
// field_0x14 holds the byte offset within the current block.
#define VIBUF_BLOCK_SHIFT 11
#define VIBUF_BLOCK_SIZE (1 << VIBUF_BLOCK_SHIFT)

#endif
