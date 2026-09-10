#include "common.h"
#include "menu.h"

extern "C" int menu_pointIsClockwise(int a, int b, int c, int d, int e, int f) {
    register int x asm("$4") = a - c;
    asm volatile("" : "+r"(x));
    register int y asm("$5") = b - d;
    asm volatile("" : "+r"(y));
    register int dx asm("$2") = e - c;
    asm volatile("" : "+r"(dx));
    register int dy asm("$9") = f - d;
    dx *= y;
    dy *= x;
    dx -= dy;
    int r = dx;
    if (r < 0)
        return 1;
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu_callbacks", func_00208840);

extern "C" void menu_restoreSelection(void) {
    MenuState* menu = &menuStateData;
    *(int*)0x15EEB0 = 3;
    register int selected asm("$4");
    selected = menu->saved;
    menu->selected = selected;
    menu->pending = 0;
}
