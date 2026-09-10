#include "common.h"
#include "vibuf.h"

// C linkage: these kernel exports are unmangled SDK entry points.
extern "C" int CreateSema(int a);
extern "C" int SignalSema(int a);
extern "C" int WaitSema(int a);

u32 getFIFOindex(ViBuf* self, void* v) {
    u32 i = ((self->blocks << 4) + self->tagBase + 0x10) & 0xFFFFFFF;
    if (v == (void*)i) {
        return 0;
    }
    return ((u32)v - self->base) >> 11;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", setD3_CHCR__FUi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", setD4_CHCR__FUi);

extern "C" void scTag2(u64* tag, u32 address, u32 id, u32 count) {
    // DMA tag: address in the upper word, tag ID at bit 28, QWC below it.
    *tag = ((u64)address << 32) | (((u64)id << 32) >> 4) | (u64)count;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufCreate);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufReset__FP5ViBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufBeginPut__FP5ViBufPPUcPiT1T2);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufEndPut__FP5ViBufi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufAddDMA__FP5ViBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufStopDMA__FP5ViBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufRestartDMA__FP5ViBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufDelete__FP5ViBuf);

int viBufCount(ViBuf* self) {
    WaitSema(self->sema);
    int x = (self->field_0x10 << 11) + self->field_0x14;
    SignalSema(self->sema);
    return x;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufFlush__FP5ViBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufModifyPts__FP5ViBufP9TimeStamp);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufPutTs__FP5ViBufP9TimeStamp);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufGetTs__FP5ViBufP9TimeStamp);
