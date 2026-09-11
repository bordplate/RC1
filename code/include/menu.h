#ifndef RC1_MENU_H
#define RC1_MENU_H

struct MenuState {
    char pad0[0x1C];
    int selected;
    char pad1[0xBC - 0x20];
    int saved;
    char pad2[0xDC - 0xC0];
    int field_0xDC;
    int field_0xE0;
    char pad3[0xF4 - 0xE4];
    int pending;
};

extern MenuState menuStateData __attribute__((section(".data")));
extern int menuPostCallbackIndex __attribute__((section(".data")));
extern int menuPostFlags __attribute__((section(".data")));
extern int menu_postHasPendingSelection __attribute__((section(".data")));

#define MENU_POST_CALLBACK_INDEX_ADDRESS 0x15EEB0
#define MENU_POST_SUBMENU_ENABLED_FLAG 0x1
#define MENU_POST_PAGE_OPEN_BLOCKED_FLAG 0x40
#define MENU_POST_SELECT_NEXT_PAGE_CALLBACK 3
#define MENU_POST_PROCESS_PAGE_SELECTION_CALLBACK 4
#define MENU_POST_UNSET_PAGE_INDEX -1

#endif
