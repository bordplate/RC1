#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", Hud_GetIconIndex__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FEE88);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", LinkHudBank__FiPc);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF120);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", Hud_SendResidentBank__FiPcb);

typedef struct {
    u8 pad[0x10];
    u32 heapCursor;
    u32 heapEnd;
} HudHeap;

extern int hudHeapBase __attribute__((section(".data")));
extern HudHeap hudHeap __attribute__((section(".data")));

void Hud_HeapReset(void) {
    hudHeap.heapEnd = hudHeapBase + 0x64000;
    hudHeap.heapCursor = hudHeapBase;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", Hud_HeapAlloc__FUiPcT1i);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF308);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF418);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF480);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF500);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF568);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF570);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF5E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF6D8);

extern int hudMessageTimer;

void hud_updateMessageTimer(void) {
    if (hudMessageTimer != 0) {
        hudMessageTimer--;
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF780);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF958);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", GetIconFrame__Fii);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", GetFrameTex__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FFC30);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FFE18);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200078);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200258);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200468);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200600);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200958);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", Hud_sendTexture__FPciiiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200C80);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200E08);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200F90);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00201110);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00201128);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00201200);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_002012A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_002012B8);
