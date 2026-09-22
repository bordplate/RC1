#include "common.h"
#include "pad_state.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/pad", UpdatePad__FR3PAD);

INCLUDE_ASM("code/_generated/nonmatchings/game/pad", ClearPadInput__FR3PAD);

INCLUDE_ASM("code/_generated/nonmatchings/game/pad", ProcessPadInput__FR3PADPUci);

void UpdatePad(PAD& self);

void UpdatePad(void) {
    UpdatePad(padState);
}
