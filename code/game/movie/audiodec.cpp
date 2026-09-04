#include "common.h"
#include "types.h"

typedef struct _AudioDec {
    u8 _pad[0x50];
    int sentPos;
} _AudioDec;

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecCreate__FP9_AudioDecPUci14sceMpegStrType);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecDelete__FP9_AudioDec);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", func_0023ACB0);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecStart);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecReset__FP9_AudioDec);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecBeginPut__FP9_AudioDecPPUcPiT1T2);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecEndPut__FP9_AudioDeci);

extern "C" int audioDecIsPageFull(_AudioDec* self) {
    return self->sentPos >= 0x1000;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecSend);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", sendToSPU__FP9_AudioDecPUcii);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", sendADPCM__FP9_AudioDec);
