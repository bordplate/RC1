#include "common.h"
#include "menu.h"

void menu_post_openMap(void) {
    if (menuPostFlags & MENU_POST_PAGE_OPEN_BLOCKED_FLAG) {
        return;
    }
    menuPostCallbackIndex = MENU_POST_SELECT_NEXT_PAGE_CALLBACK;
}

void menu_post_openMissions(void) {
    if (menuPostFlags & MENU_POST_PAGE_OPEN_BLOCKED_FLAG) {
        return;
    }
    menuPostCallbackIndex = MENU_POST_SELECT_NEXT_PAGE_CALLBACK;
}

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_pages_end", func_00208F28);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_pages_end", func_00208FA0);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_pages_end", func_00208FE8);
