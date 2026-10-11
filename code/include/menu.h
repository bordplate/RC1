#ifndef RC1_MENU_H
#define RC1_MENU_H

// The async memory-card I/O state machine (g_memcard in the Deadlocked
// reference). The boot save/load "menu" is a front end for it: the page
// callbacks below set nextstage/nextslot to request an operation and wait
// for stage to return to idle. Field names match the memcard machine; the
// struct/global name and the remaining fields are still placeholders.
// See notes/menu_post_mid_menu_post_pageAdvance.md.
struct MenuState {
    char pad0[0x1C];
    int selected;
    char pad1[0xBC - 0x20];
    int saved;
    char pad2[0xD4 - 0xC0];
    int stage;
    int substage;
    int nextstage;
    int nextslot;
    char pad3[0xF4 - 0xE4];
    int pending;
};

extern MenuState menuStateData __attribute__((section(".data")));
extern int menuPostCallbackIndex __attribute__((section(".data")));
extern int menuPostFlags __attribute__((section(".data")));
// menuStateData.selected viewed as a standalone global: 0x13D2AC is
// 0x13D290 + 0x1C. Both spellings are required in source; the direct
// symbol loads and the struct-field loads compile to different register
// forms that the original uses in different functions.
extern int menu_postHasPendingSelection __attribute__((section(".data")));

#define MENU_POST_SUBMENU_ENABLED_FLAG 0x1
// menuPostFlags bits whose meaning is not yet established.
#define MENU_POST_FLAG_02 0x2
#define MENU_POST_FLAG_04 0x4
#define MENU_POST_PAGE_OPEN_BLOCKED_FLAG 0x40
// Sentinel compared against menuStateData.selected by the post callbacks.
#define MENU_POST_SELECTION_NONE -2
#define MENU_POST_SELECT_NEXT_PAGE_CALLBACK 3
#define MENU_POST_PROCESS_PAGE_SELECTION_CALLBACK 4
// nextstage request: format the card if unformatted (memcard stage 3).
#define MEMCARD_REQUEST_FORMAT 3
#define MENU_POST_OPEN_PAGE_CALLBACK 8
// Post-callback table slots whose handlers are still INCLUDE_ASM.
#define MENU_POST_FLAG_RESOLVE_CALLBACK 6
#define MENU_POST_PAGE_ADVANCE_CALLBACK 10
#define MENU_POST_BOOT_FLAG_RESOLVE_CALLBACK 13
#define MENU_POST_UNSET_PAGE_INDEX -1
// stage value of the memcard machine: 2 is idle (it will pick up a pending
// nextstage/nextslot request on its next tick). The page-open callback
// (slot 8) and the page-advance callback (slot 10) test for it before they
// act; bmain spins memcard_Update while stage >= 3 (an operation is running)
// or nextstage >= 0 (a request is pending). Stages 0/1 are the probe/record
// cycle and 3..23 are the active operations (see the jtbl dispatch in
// memcard_Update).
#define MEMCARD_STAGE_IDLE 2
// nextstage request: scan/getdir the card (memcard stage 7). Written by the
// page-advance callback (slot 10).
#define MEMCARD_REQUEST_SCAN 7
// Slot 11 of the post-callback table, queued by the page-advance callback
// (slot 10) and the page-open callback (slot 8). After a page opens or
// advances it routes the chain to the next step based on the just-read
// memcard slot. The handler (0x208C00) is still INCLUDE_ASM.
#define MENU_POST_PAGE_OPEN_DECISION_CALLBACK 11

#endif
