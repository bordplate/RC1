#include "common.h"
#include "types.h"
#include "hud.h"

extern int hudMessageTimer;

void Hud_SetupChannelIcon(HudChanSlot* slot, int iconId) {
    int index = Hud_GetIconIndex(iconId);
    slot->iconId = ((HudIconDef*)hudHeap.iconTable)[index].id;
    slot->iconIndex = index;
    slot->iconAnimType = ((HudIconDef*)hudHeap.iconTable)[index].animType;
    slot->iconStart = ((HudIconDef*)hudHeap.iconTable)[index].start;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_001FF568);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_001FF570);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_001FF5E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_001FF6D8);

void hud_updateMessageTimer(void) {
    if (hudMessageTimer != 0) {
        hudMessageTimer--;
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_001FF780);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_001FF958);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", GetIconFrame__Fii);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", GetFrameTex__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_001FFC30);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_001FFE18);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_00200078);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_00200258);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_00200468);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_00200600);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_00200958);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", Hud_sendTexture__FPciiiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_00200C80);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_00200E08);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_00200F90);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_00201110);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_00201128);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_00201200);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", func_002012A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post", draw_bootImage__Fi);
