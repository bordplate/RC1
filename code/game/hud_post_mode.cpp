#include "common.h"
#include "types.h"
#include "hud.h"

void Hud_SetChannelModeBySerial(int serial, int mode) {
    int i = 0;
    while (i < HUD_SLOT_COUNT && hudChanSlots[i].serial != serial) {
        i++;
    }
    if (i < HUD_SLOT_COUNT) {
        hudChanSlots[i].pendMode = mode;
        if (hudChanSlots[i].pending == 0) {
            hudChanSlots[i].mode = mode;
        }
    }
}

// This padding nop preserves the original boundary before the next generated
// HUD function (0x1FF5E8).
asm("nop");
