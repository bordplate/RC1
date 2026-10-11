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
    menuStateData.nextslot = MENU_POST_UNSET_PAGE_INDEX;
    menuPostCallbackIndexGp = MENU_POST_PROCESS_PAGE_SELECTION_CALLBACK;
    menuStateData.nextstage = MENU_POST_UNSET_PAGE_INDEX;
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
    int pendingStage = menuStateData.nextstage;
    menuStateData.selected = 0;
    if (pendingStage < 0) {
        menuStateData.nextslot = 0;
        menuStateData.nextstage = MEMCARD_REQUEST_FORMAT;
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

// Page-advance callback (post-callback table slot 10), queued by
// menu_post_gatePageOpen when MENU_POST_FLAG_02|0x4 are set. When the memcard
// machine is idle (stage == MEMCARD_STAGE_IDLE) and no operation is pending
// (nextstage < 0), it requests a card scan (nextstage = MEMCARD_REQUEST_SCAN,
// nextslot = 0) and continues the chain at the post-open decision callback.
// The original keeps the state constant in a0; the pin preserves the
// constant/value register split, and the idx local forces the slot-11
// constant to materialize (into v1) before the two struct stores are emitted.
void menu_post_pageAdvance(void) {
    register int state asm("$4") = MEMCARD_STAGE_IDLE;
    if (menuStateData.stage == state && menuStateData.nextstage < 0) {
        int idx = MENU_POST_PAGE_OPEN_DECISION_CALLBACK;
        menuStateData.nextstage = MEMCARD_REQUEST_SCAN;
        menuStateData.nextslot = 0;
        menuPostCallbackIndexGp = idx;
    }
}

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
