#include "common.h"
#include "types.h"
#include "hud.h"

extern int hudMessageTimer;

// Fills the slot's icon fields (0x00, 0x40, 0x42, 0x44) from the icon-table
// entry for `iconId` (see the INCLUDE_ASM below).
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

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_001FF480);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_001FF500);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_001FF568);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_001FF570);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_001FF5E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_001FF6D8);

void hud_updateMessageTimer(void) {
    if (hudMessageTimer != 0) {
        hudMessageTimer--;
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_001FF780);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_001FF958);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", GetIconFrame__Fii);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", GetFrameTex__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_001FFC30);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_001FFE18);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_00200078);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_00200258);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_00200468);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_00200600);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_00200958);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", Hud_sendTexture__FPciiiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_00200C80);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_00200E08);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_00200F90);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_00201110);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_00201128);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_00201200);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", func_002012A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post", draw_bootImage__Fi);
