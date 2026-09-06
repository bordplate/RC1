#include "common.h"
#include "menu.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_002088D0);

extern "C" void func_00208980(void) {
    if ((*(int*)0x15EEB4 ^ 1) & 1) {
        *(int*)0x15EEB0 = 3;
    }
}

extern "C" void func_002089A8(void) {
    D_0013D290.field_0xE0 = -1;
    *(int*)0x15EEB0 = 4;
    D_0013D290.field_0xDC = -1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_002089D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208A38);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208A78);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208AF8);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208B28);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208B88);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208BC0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208C00);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208C70);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208CA8);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208D20);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208D60);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208DD8);

extern "C" void func_00208E68(void) {
    if (*(int*)0x15EEB4 & 0x40) {
        return;
    }
    *(int*)0x15EEB0 = 3;
}

extern "C" void func_00208E90(void) {
    if (*(int*)0x15EEB4 & 0x40) {
        return;
    }
    *(int*)0x15EEB0 = 3;
}

extern "C" int D_0013D2AC __attribute__((section(".data")));

extern "C" void func_00208EB8(void) {
    if (D_0013D2AC) {
        *(int*)0x15EEB0 = 3;
    }
}

extern "C" void func_00208ED8(void) {
    if (*(int*)0x15EEB4 & 0x40) {
        return;
    }
    *(int*)0x15EEB0 = 3;
}

extern "C" void func_00208F00(void) {
    if (*(int*)0x15EEB4 & 0x40) {
        return;
    }
    *(int*)0x15EEB0 = 3;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208F28);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208FA0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208FE8);
