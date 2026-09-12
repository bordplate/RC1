#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C5A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C658);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C6F8);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C7E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C830);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C8D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", sound_update);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022D708);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022D798);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022D7F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022DA68);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022DB10);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022DBA0);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022DC38);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", sound_StopAllSounds__Fv);

void sound_setBufferValue(int a, long b) {
    int c = (int)b;
    if (c)
        *(int*)c = a;
}

void sound_setBufferState(int a, long b) {
    int c = (int)b;
    if (c == 0) return;
    *(int*)c = a;
    if (a != 0) {
        if (*(u8*)(c + 4) != 1) return;
        *(u8*)(c + 4) = 2;
        return;
    }
    *(int*)(c + 0x18) = 0;
    *(int*)(c + 0x1C) = 0;
    *(u8*)(c + 4) = 0;
}

void sound_resetBufferState(int a, long b) {
    int c = (int)b;
    if (c == 0) return;
    *(int*)c = a;
    if (a != 0) return;
    *(int*)(c + 0x18) = 0;
    *(int*)(c + 0x1C) = 0;
    *(char*)(c + 4) = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022DE08);
