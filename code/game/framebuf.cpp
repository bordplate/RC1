#include "common.h"
#include "types.h"

extern s64 D_152078 __attribute__((section(".data")));

//INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", SetBackgroundColor_Fiii);
void SetBackgroundColor(s32 r, s32 g, s32 b) {
    D_152078 = (long)r | ((long)g << 8) | ((long)b << 0x10) | (0x8000ULL << 0x10);
}
