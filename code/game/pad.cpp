#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/pad", UpdatePad__FR3PAD);

INCLUDE_ASM("code/_generated/nonmatchings/game/pad", ClearPadInput__FR3PAD);

INCLUDE_ASM("code/_generated/nonmatchings/game/pad", ProcessPadInput__FR3PADPUci);

struct PAD;

extern PAD D_0013C940;

void UpdatePad(PAD& self);

void UpdatePad(void) {
    UpdatePad(D_0013C940);
}
