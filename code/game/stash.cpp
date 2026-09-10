#include "common.h"

// C linkage: the sound-system entry point is exported by an unmangled assembly symbol.
extern "C" void initializeSoundSystem(int mode);

void stash_init(void) {
    initializeSoundSystem(0);
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

int stash_getEntryValue(unsigned entry) {
    if (entry >= 0x40) {
        return -3;
    }
    return stashEntries[entry].f1;
}

// These padding instructions preserve the original boundary after the final
// decompiled stash helper.
asm("nop");
asm("nop");
