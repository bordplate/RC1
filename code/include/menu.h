#ifndef RC1_MENU_H
#define RC1_MENU_H

struct MenuState {
    char pad0[0x1C];
    int selected;
    char pad1[0xBC - 0x20];
    int saved;
    char pad2[0xD4 - 0xC0];
    int field_0xD4;
    char pad2b[0xDC - 0xD8];
    int field_0xDC;
    int field_0xE0;
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
#define MENU_POST_PENDING_PAGE_RESOLVED 3
#define MENU_POST_OPEN_PAGE_CALLBACK 8
// Post-callback table slots whose handlers are still INCLUDE_ASM.
#define MENU_POST_FLAG_RESOLVE_CALLBACK 6
#define MENU_POST_PAGE_ADVANCE_CALLBACK 10
#define MENU_POST_BOOT_FLAG_RESOLVE_CALLBACK 13
#define MENU_POST_UNSET_PAGE_INDEX -1
// menuStateData.field_0xD4 is the menu page state. 2 is the state tested by
// the page-open callback (slot 8) and the page-advance callback (slot 10)
// before they act; bmain polls while the state is >= 3. The full enum
// (0/1/2/>=3) is not yet decoded, so the name records the value, not a
// confirmed meaning.
#define MENU_POST_PAGE_STATE_2 2
// Values stored in the pending-page word (menuStateData.field_0xDC): -1 =
// unset (MENU_POST_UNSET_PAGE_INDEX), 3 = resolved, 7 = written by the
// page-advance callback (slot 10). The meaning of 7 is not yet established.
#define MENU_POST_PENDING_PAGE_7 7
// Slot 11 of the post-callback table, queued by the page-advance callback
// (slot 10) and the page-open callback (slot 8). The handler (0x208C00) is
// still INCLUDE_ASM, so the name records the slot, not a confirmed role.
#define MENU_POST_CALLBACK_SLOT_11 11

#endif
