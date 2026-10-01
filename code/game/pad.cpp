#include "common.h"
#include "pad_state.h"

void ClearPadInput(PAD& self);

INCLUDE_ASM("code/_generated/nonmatchings/game/pad", UpdatePad__FR3PAD);

void ClearPadInput(PAD& self) {
    self.field_1B0 = 0;
    self.field_1A0 = 0;
    self.pressedButtons = 0;
    self.field_1A8 = 0;
    self.field_1D0 = 1;
    self.field_1D4 = 1;
    self.field_1B4 = 0;
    self.field_1B8 = 0;
    self.field_1C0 = 0;
    self.field_1C4 = 0;
    self.field_1C8 = 0;
    self.field_1D8 = 0;
    for (int i = 0; i < 16; i++) {
        self.analog[i] = 0.0f;
        self.hudAnalog[i] = 0.0f;
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pad", ProcessPadInput__FR3PADPUci);

void UpdatePad(PAD& self);

void UpdatePad(void) {
    UpdatePad(padState);
}
