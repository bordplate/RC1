#include "common.h"
#include "vibuf.h"

extern "C" int func_00118980(int a);
extern "C" int func_00118990(int a);
extern "C" int func_001189B0(int a);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", getFIFOindex__FP5ViBufPv);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", setD3_CHCR__FUi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", setD4_CHCR__FUi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", scTag2);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufCreate);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufReset__FP5ViBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufBeginPut__FP5ViBufPPUcPiT1T2);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufEndPut__FP5ViBufi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufAddDMA__FP5ViBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufStopDMA__FP5ViBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufRestartDMA__FP5ViBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufDelete__FP5ViBuf);

int viBufCount(ViBuf* self) {
    func_001189B0(self->sema);
    int x = (self->field_0x10 << 11) + self->field_0x14;
    func_00118990(self->sema);
    return x;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufFlush__FP5ViBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufModifyPts__FP5ViBufP9TimeStamp);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufPutTs__FP5ViBufP9TimeStamp);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vibuf", viBufGetTs__FP5ViBufP9TimeStamp);
