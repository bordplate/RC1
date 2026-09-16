#include "common.h"

// C linkage: this callback is installed through the original unmangled menu
// jump table entry.
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

// Blocked: the menuPostCallbackIndex store only matches as a constant-address
// cast, and static data addresses are not allowed in source (overlay
// compatibility). The generated assembly references the symbol, so the
// fallback is overlay-safe. See decomp_state/notes/menu_func_002088A8.md.
INCLUDE_ASM("code/_generated/nonmatchings/game/menu_callbacks", menu_restoreSelection);
