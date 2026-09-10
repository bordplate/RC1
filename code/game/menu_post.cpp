#include "common.h"
#include "menu.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_002088D0);

void menu_post_enableSubmenu(void) {
    if ((*(int*)0x15EEB4 ^ 1) & 1) {
        *(int*)0x15EEB0 = 3;
    }
}

void menu_post_selectNextPage(void) {
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

void menu_post_openInventory(void) {
    if (*(int*)0x15EEB4 & 0x40) {
        return;
    }
    *(int*)0x15EEB0 = 3;
}

void menu_post_openWeapons(void) {
    if (*(int*)0x15EEB4 & 0x40) {
        return;
    }
    *(int*)0x15EEB0 = 3;
}

extern int menu_postHasPendingSelection __attribute__((section(".data")));

void menu_post_openGadgets(void) {
    if (menu_postHasPendingSelection) {
        *(int*)0x15EEB0 = 3;
    }
}

void menu_post_openMap(void) {
    if (*(int*)0x15EEB4 & 0x40) {
        return;
    }
    *(int*)0x15EEB0 = 3;
}

void menu_post_openMissions(void) {
    if (*(int*)0x15EEB4 & 0x40) {
        return;
    }
    *(int*)0x15EEB0 = 3;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208F28);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208FA0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_00208FE8);
