#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", func_001E8D00);

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", actuator_CalcPower);

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", func_001E9120);

extern "C" int D_0015EE74;

void texResetCursor(void) {
    D_0015EE74 = *(int *)0x15EE8C;
    *(int *)0x15EF20 = 0;
}

asm("nop");
asm("nop");

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", func_001E9148);
