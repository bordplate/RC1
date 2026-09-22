#ifndef PAD_STATE_H
#define PAD_STATE_H

#include "types.h"

// First controller state at 0x13C940, passed by UpdatePad to the pad reader.
// Historically called streamState; +0x1A4 is the newly pressed button mask.
struct PAD {
    u8 pad_0x00[0x18E];
    s16 field_18e;
    u32 field_190;
    u8 pad_0x194[0x10];
    int pressedButtons;
};

extern PAD padState __attribute__((section(".data")));

#endif
