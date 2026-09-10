#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", func_001E8D00);

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", actuator_CalcPower);

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", func_001E9120);

extern int textureCursor;

void texResetCursor(void) {
    textureCursor = *(int*)0x15EE8C;
    *(int*)0x15EF20 = 0;
}

// These padding instructions preserve the original boundary before the next
// generated actuator function.
asm("nop");
asm("nop");

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", func_001E9148);
