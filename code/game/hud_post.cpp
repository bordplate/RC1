#include "common.h"
#include "types.h"
#include "hud.h"

// Commits a pending channel request: sets up the slot icon from pendId, copies
// the pending block into the active block, invokes the committed callback when
// it is non-zero, and clears the pending flag. The `fn` store is unconditional
// (it lands in the beqz delay slot); only the callback call is conditional.
void Hud_CommitChannel(HudChanSlot* slot) {
    Hud_SetupChannelIcon(slot, slot->pendId);
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
