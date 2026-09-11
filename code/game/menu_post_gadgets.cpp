#include "common.h"
#include "menu.h"

void menu_post_openGadgets(void) {
    if (menu_postHasPendingSelection) {
        // A symbolic store changes EEGCC's address register and store schedule here.
        *(int*)MENU_POST_CALLBACK_INDEX_ADDRESS = MENU_POST_SELECT_NEXT_PAGE_CALLBACK;
    }
}
