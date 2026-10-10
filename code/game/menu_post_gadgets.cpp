#include "common.h"
#include "menu.h"

// Plain in-window alias for menuPostCallbackIndex (config/linker_aliases.ld):
// the original store is the absolute `lui at; sw` pair that the plain
// declaration expands to outside the epilogue noreorder block.
extern int menuPostCallbackIndexGp;

void menu_post_openGadgets(void) {
    if (menu_postHasPendingSelection) {
        menuPostCallbackIndexGp = MENU_POST_SELECT_NEXT_PAGE_CALLBACK;
    }
}
