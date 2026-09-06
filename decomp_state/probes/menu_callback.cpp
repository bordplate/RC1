struct MenuData {
    char pad0[0x1C];
    int selected;
    char pad1[0xBC - 0x20];
    int saved;
    char pad2[0xF4 - 0xC0];
    int pending;
};
extern "C" MenuData D_0013D290 __attribute__((section(".data")));

extern "C" void menu_restoreSelection(void) {
    MenuData* menu = &D_0013D290;
    *(int*)0x15EEB0 = 3;
    register int selected asm("$4");
    selected = menu->saved;
    menu->selected = selected;
    menu->pending = 0;
}
