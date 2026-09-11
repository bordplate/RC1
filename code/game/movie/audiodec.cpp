#include "common.h"
#include "types.h"

// Audio decoder state. Every observed member is a 4-byte int, so fields are
// named by offset and only the unobserved gaps are pad. audioDecReset zeros
// the listed int fields; the sibling generated functions (audioDecCreate,
// audioDecStart, audioDecBeginPut, audioDecEndPut, sendToSPU, sendADPCM)
// establish the remaining offsets.
typedef struct _AudioDec {
    int field_0x00;
    int field_0x04;
    u8 pad_08[0xC];
    int field_0x14;
    int field_0x18;
    int field_0x1C;
    u8 pad_20[0x10];
    int field_0x30;
    int field_0x34;
    int field_0x38;
    int field_0x3C;
    int field_0x40;
    int field_0x44;
    int field_0x48;
    int field_0x4C;
    int sentPos;
    u8 pad_54[4];
    int field_0x58;
    int field_0x5C;
    int field_0x60;
} _AudioDec;

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecCreate__FP9_AudioDecPUci14sceMpegStrType);

// C linkage: this sound-library routine is an unmangled generated entry point.
extern "C" void snd_CloseMovieSound(void);

// C linkage: this sound-library routine is an unmangled generated entry point.
extern "C" void snd_ResetMovieSound(void);

int audioDecDelete(_AudioDec* self) {
    snd_CloseMovieSound();
    return 1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", func_0023ACB0);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecStart);

void audioDecReset(_AudioDec* self) {
    snd_ResetMovieSound();

    self->field_0x00 = 0;
    self->field_0x30 = 0;
    self->field_0x38 = 0;
    self->field_0x3C = 0;
    self->field_0x44 = 0;
    self->sentPos = 0;
    self->field_0x58 = 0;
    self->field_0x5C = 0;
}

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
