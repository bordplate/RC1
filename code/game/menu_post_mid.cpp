#include "common.h"
#include "menu.h"

// The original callback-index stores in this file mix access modes on
// 0x15EEB0: a single GP-relative `sw v0,-0x7d50(gp)` when the store lands in
// an epilogue noreorder delay slot, and an absolute `lui at; sw` pair
// elsewhere. A plain in-window declaration expands to exactly those two
// forms, while menu.h's section(".data") declaration of
// menuPostCallbackIndex compiles to a split with a general-register base.
// The linker aliases (config/linker_aliases.ld) map these to the same
// globals; menuPostFlagsGp works the same way for the GPREL flag loads that
// sit in branch delay slots.
extern int menuPostCallbackIndexGp;
extern int menuPostFlagsGp;

void menu_post_selectNextPage(void) {
    menuStateData.field_0xE0 = MENU_POST_UNSET_PAGE_INDEX;
    menuPostCallbackIndexGp = MENU_POST_PROCESS_PAGE_SELECTION_CALLBACK;
    menuStateData.field_0xDC = MENU_POST_UNSET_PAGE_INDEX;
}

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_002089D0);

// Gates the page-selection flow: while the pending selection is not the
// MENU_POST_SELECTION_NONE sentinel the chain continues with the
// select-next-page callback; otherwise a menuPostFlags bit routes to the
// flag-resolve callback (table slot 6).
void menu_post_gatePendingSelection(void) {
    // The original keeps the -2 constant in a0; the pin preserves the
    // constant/value register split.
    register int sentinel asm("$4") = MENU_POST_SELECTION_NONE;
    if (menu_postHasPendingSelection != sentinel) {
        menuPostCallbackIndexGp = MENU_POST_SELECT_NEXT_PAGE_CALLBACK;
        return;
    }
    if (menuPostFlagsGp & MENU_POST_FLAG_02)
        menuPostCallbackIndexGp = MENU_POST_FLAG_RESOLVE_CALLBACK;
}

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208A78);

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

// Gates the page-open flow: while the pending selection is active the chain
// continues with the select-next-page callback; otherwise the
// MENU_POST_FLAG_02/0x4 bits route to the page-advance callback
// (table slot 10).
void menu_post_gatePageOpen(void) {
    if (menu_postHasPendingSelection) {
        menuPostCallbackIndexGp = MENU_POST_SELECT_NEXT_PAGE_CALLBACK;
        return;
    }
    if (menuPostFlagsGp & (MENU_POST_FLAG_02 | MENU_POST_FLAG_04))
        menuPostCallbackIndexGp = MENU_POST_PAGE_ADVANCE_CALLBACK;
}

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208BC0);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208C00);

// Gates the boot-level flow: while the pending selection is active the chain
// continues with the select-next-page callback; otherwise the
// MENU_POST_FLAG_02 bit routes to the boot flag-resolve callback
// (table slot 13).
void menu_post_gateBootSelection(void) {
    if (menu_postHasPendingSelection) {
        menuPostCallbackIndexGp = MENU_POST_SELECT_NEXT_PAGE_CALLBACK;
        return;
    }
    if (menuPostFlagsGp & MENU_POST_FLAG_02)
        menuPostCallbackIndexGp = MENU_POST_BOOT_FLAG_RESOLVE_CALLBACK;
}

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208CA8);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208D20);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208D60);

// Only placement in the menu-post callback range is established; behavior remains uninvestigated.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_post_mid", func_00208DD8);
