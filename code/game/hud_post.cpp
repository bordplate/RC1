#include "common.h"
#include "types.h"
#include "hud.h"

// Fills the slot's icon fields (0x00, 0x40, 0x42, 0x44) from the icon-table
// entry for `iconId` (see the INCLUDE_ASM in hud_post_post.cpp).
void func_001FF500(HudChanSlot*, int) asm("func_001FF500");

// Commits a pending channel request: sets up the slot icon from pendId, copies
// the pending block into the active block, invokes the committed callback when
// it is non-zero, and clears the pending flag. The `fn` store is unconditional
// (it lands in the beqz delay slot); only the callback call is conditional.
void Hud_CommitChannel(HudChanSlot* slot) {
    func_001FF500(slot, slot->pendId);
    slot->mode = slot->pendMode;
    slot->d = slot->pendD;
    slot->e = slot->pendE;
    slot->c = slot->pendC;
    slot->b = slot->pendB;
    slot->fn = slot->pendFn;
    if (slot->pendFn != 0) {
        ((void (*)(HudChanSlot*))slot->fn)(slot);
    }
    slot->pending = 0;
}
