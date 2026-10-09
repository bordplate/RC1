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

// Unreachable dead tail after Hud_SetupChannelIcon (func_001FF500,
// 0x1FF500): one `addiu sp,sp,0x40`; the unit size does not match the
// parent's 0x20 frame (see the 989snd dead addiu-sp tail family). Nothing
// reaches 0x1FF568 (0 jal/j/branch/data references; no Ghidra function), so
// it is not a function; the original compiler emitted this byte after the
// parent's RTL, so it is preserved here as an exact word. The trailing nop
// pads to the 8-aligned func_001FF570 entry.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001FF568, 0x4\n"
    "glabel func_001FF568\n"
    "    .word 0x27bd0040\n"
    "endlabel func_001FF568\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

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
