#include "common.h"
#include "types.h"

typedef struct {
    u8 pad[0x34];
    u32 actionList;
} PauseActionMode;

extern int pauseActionListA[];
extern int pauseActionListB[];
extern int pauseActionListMode;

extern "C" int SetPauseActionList(PauseActionMode* mode) {
    int listMode;
    asm volatile("lw %0, pauseActionListMode" : "=r"(listMode));
    if (listMode != 0)
        mode->actionList = (u32)pauseActionListA;
    else
        mode->actionList = (u32)pauseActionListB;
    return 0;
}
