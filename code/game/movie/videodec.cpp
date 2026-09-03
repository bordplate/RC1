#include "common.h"
#include "types.h"

typedef struct VideoDec {
    u8 _pad[0xA8];
    u32 state;
} VideoDec;

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecCreate__FP8VideoDecPUciPUxT3iP9TimeStampi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023CBC8);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecSetStream__FP8VideoDeciiPFP7sceMpegP13sceMpegCbDataPv_iPv);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecBeginPut__FP8VideoDecPPUcPiT1T2);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecEndPut__FP8VideoDec);

void videoDecReset(VideoDec* self) {
    self->state = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecDelete__FP8VideoDec);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecAbort__FP8VideoDec);

extern "C" int videoDecGetState(VideoDec* self) {
    return self->state;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecSetState__FP8VideoDecUi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecPutTs__FP8VideoDecllPUci);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecInputCount__FP8VideoDec);

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
