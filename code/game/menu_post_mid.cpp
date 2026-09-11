#include "common.h"
#include "menu.h"

void menu_post_selectNextPage(void) {
    menuStateData.field_0xE0 = MENU_POST_UNSET_PAGE_INDEX;
    // A symbolic store changes EEGCC's address register and store schedule here.
    *(int*)MENU_POST_CALLBACK_INDEX_ADDRESS = MENU_POST_PROCESS_PAGE_SELECTION_CALLBACK;
    menuStateData.field_0xDC = MENU_POST_UNSET_PAGE_INDEX;
}

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_002089D0);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208A38);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208A78);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208AF8);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208B28);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208B88);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208BC0);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208C00);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208C70);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208CA8);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208D20);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208D60);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208DD8);
