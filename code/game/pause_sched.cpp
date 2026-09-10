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

void pause_scheduleInput(void)
{
    register PauseScreenState* base asm("$3") = &pauseScreenState;

    pauseScreenState.f0 = 0x2D;

    register u32 mode asm("$5") = 3;

    base->f110 = 0;
    *(u32*)0x15F604 = mode;
    base->fC = 0;
    base->f10 = 0;
}
