#ifndef PAD_STATE_H
#define PAD_STATE_H

#include "types.h"

// First controller state at 0x13C940, passed by UpdatePad to the pad reader.
// Historically called streamState; +0x1A4 is the newly pressed button mask.
struct PAD {
    u8 pad_0x00[0x100];
    // 0x100/0x140: ProcessPadInput fills 0x100 from the raw pad bytes with
    // float math and then copies the 16 floats verbatim to 0x140. Named after
    // the equivalent analog[]/hudAnalog[] members in the Deadlocked PAD struct.
    f32 analog[16];         // 0x100
    f32 hudAnalog[16];      // 0x140
    u8 pad_0x180[0xE];
    s16 field_18e;          // 0x18E
    u32 field_190;          // 0x190
    u8 pad_0x194[0xC];
    u32 field_1A0;          // 0x1A0
    int pressedButtons;     // 0x1A4: newly pressed button mask
    u32 field_1A8;          // 0x1A8
    u32 field_1AC;          // 0x1AC
    u32 field_1B0;          // 0x1B0
    u32 field_1B4;          // 0x1B4
    u32 field_1B8;          // 0x1B8
    u32 field_1BC;          // 0x1BC
    u32 field_1C0;          // 0x1C0
    u32 field_1C4;          // 0x1C4
    u32 field_1C8;          // 0x1C8
    u32 field_1CC;          // 0x1CC
    u32 field_1D0;          // 0x1D0
    u32 field_1D4;          // 0x1D4
    u32 field_1D8;          // 0x1D8
    u32 field_1DC;          // 0x1DC
};

extern PAD padState __attribute__((section(".data")));

#endif
