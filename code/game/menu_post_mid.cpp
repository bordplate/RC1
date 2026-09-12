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

// The original callback-index store here is a single GP-relative
// `sw v0,-0x7d50(gp)`, which only a plain in-window declaration produces;
// menu.h's section(".data") declaration of menuPostCallbackIndex compiles
// to an absolute lui/sw pair. The linker alias (config/linker_aliases.ld)
// maps this to the same global.
extern int menuPostCallbackIndexGp;

void menu_post_preparePageOpen(void) {
    // The condition must be read through a local: direct field access makes
    // EEGCC allocate the struct base to v1 and the value to v0 instead of
    // the original base=a0/value=v1.
    int pendingPage = menuStateData.field_0xDC;
    menuStateData.selected = 0;
    if (pendingPage < 0) {
        menuStateData.field_0xE0 = 0;
        menuStateData.field_0xDC = MENU_POST_PENDING_PAGE_RESOLVED;
    }
    menuPostCallbackIndexGp = MENU_POST_OPEN_PAGE_CALLBACK;
}

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
