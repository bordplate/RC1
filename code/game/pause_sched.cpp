#include "common.h"
#include "types.h"

typedef struct {
    u32 f0;
    u8 pad4[8];
    u32 fC;
    u32 f10;
    u8 pad[0xFC];
    u32 f110;
} PauseScreenState;

extern PauseScreenState pauseScreenState __attribute__((section(".data")));

// Match-sensitive: plain extern (no .data section attribute). EGC emits one
// store pseudo that ps2eeas expands in place to the original's
// lui at / sw pair.
// Pinned pause/game mode; pause_scheduleInput forces it to 3.
extern u32 GameMode;

void pause_scheduleInput(void) {
    register PauseScreenState* base asm("$3") = &pauseScreenState;

    pauseScreenState.f0 = 0x2D;

    register u32 mode asm("$5") = 3;

    base->f110 = 0;
    GameMode = mode;
    base->fC = 0;
    base->f10 = 0;
}
