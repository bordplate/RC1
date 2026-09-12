#ifndef AUDIODEC_H
#define AUDIODEC_H

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

// Offset of the _AudioDec state inside the movie decode buffer (the
// movieDecodeBuf base).
#define MOVIE_AUDIO_DEC_OFFSET 0xD9100

// C linkage: these decoder callbacks are referenced by the original unmangled
// movie-decoder API.
extern "C" int audioDecIsPageFull(_AudioDec* self);
extern "C" void audioDecSend(_AudioDec* self);

#endif
