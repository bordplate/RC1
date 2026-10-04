#include "common.h"
#include "types.h"
#include "audiodec.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", audioDecCreate__FP9_AudioDecPUci14sceMpegStrType);

// C linkage: this sound-library routine is an unmangled generated entry point.
extern "C" void snd_CloseMovieSound(void);

// C linkage: this sound-library routine is an unmangled generated entry point.
extern "C" void snd_ResetMovieSound(void);

// C linkage: this sound-library routine is an unmangled generated entry point.
extern "C" void snd_StartMovieSound(int iopBuffer, int iopBufferSize,
                                    int iopPausePosition, int sr, int ch);

int audioDecDelete(_AudioDec* self) {
    snd_CloseMovieSound();
    return 1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/audiodec", func_0023ACB0);

// Start the movie sound stream: round the IOP buffer size down to a 1024-byte
// boundary (a negative size is clamped through the same rounding), issue the
// start command with the buffer, pause position, sample rate and channel count,
// then mark the decoder active (state 2).
extern "C" void audioDecStart(_AudioDec* self) {
    int size = self->field_0x4C;
    int aligned = (size > -1) ? size : size + 0x3FF;
    snd_StartMovieSound(self->field_0x48, (aligned >> 10) << 10, self->field_0x5C,
                        self->field_0x14, self->field_0x18);
    self->field_0x00 = 2;
}

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
