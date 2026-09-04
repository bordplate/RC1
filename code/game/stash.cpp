#include "common.h"

extern "C" void func_0011AB20(int param_1);

extern "C" void func_00232CE0(void) {
    func_0011AB20(0);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/stash", func_00232D00);

INCLUDE_ASM("code/_generated/nonmatchings/game/stash", Stash_SendData);

INCLUDE_ASM("code/_generated/nonmatchings/game/stash", func_00232F20);

INCLUDE_ASM("code/_generated/nonmatchings/game/stash", func_00233038);
