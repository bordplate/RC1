#include "common.h"

extern "C" void func_0011AB20(int param_1);

extern "C" void func_00232CE0(void) {
    func_0011AB20(0);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/stash", func_00232D00);

INCLUDE_ASM("code/_generated/nonmatchings/game/stash", Stash_SendData);

INCLUDE_ASM("code/_generated/nonmatchings/game/stash", func_00232F20);

typedef struct {
    int f0;
    int f1;
    int f2;
    int f3;
} StashEntry16;

extern "C" StashEntry16 D_001DD1D8[64];

extern "C" int func_00233038(unsigned param_1) {
    if (param_1 >= 0x40) {
        return -3;
    }
    return D_001DD1D8[param_1].f1;
}

asm("nop");
asm("nop");
