#include "common.h"
#include "menu.h"

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post", func_002088D0);

void menu_post_enableSubmenu(void) {
    if ((menuPostFlags ^ MENU_POST_SUBMENU_ENABLED_FLAG) & MENU_POST_SUBMENU_ENABLED_FLAG) {
        menuPostCallbackIndex = MENU_POST_SELECT_NEXT_PAGE_CALLBACK;
    }
}
