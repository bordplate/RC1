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
    u32 field_0x48;
    u32 field_0x4c;
    u32 tags;
    u32 tagCount;
    u32 tagHead;
    u32 tagIdx;
} ViBuf;

#endif
