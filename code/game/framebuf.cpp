#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FA860);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FA958);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", SetupFS_AA_buffer__Fiiiiii);

extern s64 frameBufferColor __attribute__((section(".data")));

void SetBackgroundColor(s32 r, s32 g, s32 b) {
    frameBufferColor = (long)r | ((long)g << 8) | ((long)b << 0x10) | (0x8000ULL << 0x10);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", PutDispBuffer__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", PutDrawBufferLarge__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB368);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", PutDrawBufferSmall__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB440);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", AA_BlurPass__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB6E0);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB740);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB8F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FBAB0);
