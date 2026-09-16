#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", func_001E8D00);

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", actuator_CalcPower);

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", func_001E9120);

extern int textureCursor;
// Base of the VRAM texture pool; textureCursor allocates from it.
extern int textureMemoryBase;
// Allocator counter cleared with the cursor at level start; no boot-ELF
// readers, likely maintained by level overlay code. Exact role unconfirmed.
extern int textureAllocCounter;

void texResetCursor(void) {
    textureCursor = textureMemoryBase;
    textureAllocCounter = 0;
}

// These padding instructions preserve the original boundary before the next
// generated actuator function.
asm("nop");
asm("nop");

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", func_001E9148);
