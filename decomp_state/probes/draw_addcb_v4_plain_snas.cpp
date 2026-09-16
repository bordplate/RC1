#include "types.h"

#define DRAW_CALLBACK_MAX 0x40

extern u32 drawCallbackFuncs[];
extern u32 drawCallbackArgs[];
extern int drawCallbackCount;

void AddDrawCallback(u32 func, u32 arg) {
    int idx = drawCallbackCount;
    if (idx < DRAW_CALLBACK_MAX) {
        drawCallbackFuncs[idx] = func;
        drawCallbackArgs[idx] = arg;
        drawCallbackCount = idx + 1;
    }
}
