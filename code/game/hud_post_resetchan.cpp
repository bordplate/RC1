#include "common.h"
#include "types.h"
#include "hud.h"

// Requests that the channel slot whose `serial` field matches `serial` be
// reset: scans the slots, and on a match asks Hud_SetChannelPending to set the
// pending id to HUD_SLOT_RESET_ICON_ID (empty slot, no callback). Returns 1
// when a slot matched, 0 otherwise. EGC peels the first slot out of the loop
// and puts the iterator increment in the back-branch delay slot.
//
// This TU uses the SN assembler (not the GNU override the sibling hud_post TUs
// keep): the compiler emits a scheduler `#nop` before the back-branch `bnel`,
// which ps2eeas expands to the original's nop but gas drops as a comment.
//
// Symbol override: the stripped ELF carries the address-based placeholder
// func_001FF480 (the original mangled name is unrecoverable), and the sibling
// INCLUDE_ASM caller func_00216C48 branches to that label.
int Hud_ResetChannelBySerial(int serial) asm("func_001FF480");

int Hud_ResetChannelBySerial(int serial) {
    int i;
    for (i = 0; i < HUD_SLOT_COUNT; i++) {
        if (hudChanSlots[i].serial == serial)
            break;
    }
    if (i < HUD_SLOT_COUNT) {
        Hud_SetChannelPending(i, HUD_SLOT_RESET_ICON_ID, 0, 0, 0, 0, 0);
        return 1;
    }
    return 0;
}
