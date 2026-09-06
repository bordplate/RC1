#ifndef RC1_MENU_H
#define RC1_MENU_H

struct MenuData_0013D290 {
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

extern "C" struct MenuData_0013D290 D_0013D290 __attribute__((section(".data")));

#endif
