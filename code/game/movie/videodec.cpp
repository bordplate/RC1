#include "common.h"
#include "types.h"

typedef struct ViBuf {
    u32 data;
    u32 chcr;
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

typedef struct VideoDec {
    u8 _pad[0x48];
    ViBuf vibuf;
    u32 state;
} VideoDec;

int viBufCount(ViBuf* buf);

int viBufBeginPut(ViBuf* buf, u8** data, int* size, u8** data2, int* size2);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecCreate__FP8VideoDecPUciPUxT3iP9TimeStampi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023CBC8);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecSetStream__FP8VideoDeciiPFP7sceMpegP13sceMpegCbDataPv_iPv);

int videoDecBeginPut(VideoDec* self, u8** data, int* size, u8** data2, int* size2) {
    return viBufBeginPut(&self->vibuf, data, size, data2, size2);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecEndPut__FP8VideoDec);

void videoDecReset(VideoDec* self) {
    self->state = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecDelete__FP8VideoDec);

void videoDecAbort(VideoDec* self) {
    self->state = 1;
}

extern "C" int videoDecGetState(VideoDec* self) {
    return self->state;
}

u32 videoDecSetState(VideoDec* self, u32 state) {
    u32 old = self->state;
    self->state = state;
    return old;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecPutTs__FP8VideoDecllPUci);

int videoDecInputCount(VideoDec* self) {
    return viBufCount(&self->vibuf);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023CD00);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecFlush__FP8VideoDec);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecIsFlushed__FP8VideoDec);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecMain__FPv);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", decBs0__FP8VideoDec);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", mpegError__FP7sceMpegP18sceMpegCbDataErrorPv);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", mpegNodata__FP7sceMpegP13sceMpegCbDataPv);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023D0E0);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023D110);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023D140);
