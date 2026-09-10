#include "common.h"

extern "C" void func_0011AB20(int param_1);

void stash_init(void) {
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

extern StashEntry16 stashEntries[64];

int stash_getEntryValue(unsigned param_1) {
    if (param_1 >= 0x40) {
        return -3;
    }
    return stashEntries[param_1].f1;
}

asm("nop");
asm("nop");
