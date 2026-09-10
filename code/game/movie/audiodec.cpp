#include "common.h"
#include "types.h"

typedef struct _AudioDec {
    u8 _pad[0x50];
    int sentPos;
} _AudioDec;

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecCreate__FP9_AudioDecPUci14sceMpegStrType);

// C linkage: this sound-library routine is an unmangled generated entry point.
extern "C" void snd_CloseMovieSound(void);

int audioDecDelete(_AudioDec* self) {
    snd_CloseMovieSound();
    return 1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", func_0023ACB0);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecStart);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecReset__FP9_AudioDec);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecBeginPut__FP9_AudioDecPPUcPiT1T2);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecEndPut__FP9_AudioDeci);

// C linkage: this decoder callback is referenced by the original unmangled
// movie-decoder API.
extern "C" int audioDecIsPageFull(_AudioDec* self) {
    return self->sentPos >= 0x1000;
}

void sendADPCM(_AudioDec* self);

// C linkage: this decoder callback is referenced by the original unmangled
// movie-decoder API.
extern "C" void audioDecSend(_AudioDec* self) {
    if (*(int*)self) {
        sendADPCM(self);
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", sendToSPU__FP9_AudioDecPUcii);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", sendADPCM__FP9_AudioDec);
