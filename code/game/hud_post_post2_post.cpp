#include "common.h"
#include "types.h"

// Unreachable dead tail the original compiler emitted after
// Hud_DrawChannels (0x1FF780, the preceding TU): one 0x10 stack
// deallocation plus the alignment nop before GetIconFrame__Fii.
// Nothing reaches 0x1FF958 (0 jal/j/branch/data references; no Ghidra
// function), so it is not a function; the original compiler emitted these
// bytes after the preceding RTL, so they are preserved here as exact words.
// EGC 2.95.2 never regenerates a dead frame deallocation after the epilogue
// (probed; see decomp_state/notes/989snd_func_0012E078.md). The glabel /
// endlabel / nonmatching structure keeps the linker-script-pinned symbols
// func_001FF958 and func_001FF958.NON_MATCHING (both at 0x1FF958) defined.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001FF958, 0x4\n"
    "glabel func_001FF958\n"
    "    .word 0x27bd0010\n"
    "endlabel func_001FF958\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", GetIconFrame__Fii);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", GetFrameTex__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_001FFC30);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_001FFE18);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_00200078);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_00200258);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_00200468);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_00200600);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_00200958);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", Hud_sendTexture__FPciiiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_00200C80);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_00200E08);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_00200F90);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_00201110);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_00201128);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_00201200);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", func_002012A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post", draw_bootImage__Fi);
