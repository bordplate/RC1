#include "common.h"
#include "menu.h"

void menu_post_openInventory(void) {
    if (menuPostFlags & MENU_POST_PAGE_OPEN_BLOCKED_FLAG) {
        return;
    }
    menuPostCallbackIndex = MENU_POST_SELECT_NEXT_PAGE_CALLBACK;
}

void menu_post_openWeapons(void) {
    if (menuPostFlags & MENU_POST_PAGE_OPEN_BLOCKED_FLAG) {
        return;
    }
    menuPostCallbackIndex = MENU_POST_SELECT_NEXT_PAGE_CALLBACK;
}
