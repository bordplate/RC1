#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FA860);

// Unreachable dead tail the original compiler emitted after
// func_001FA860 (FastMapMaskRLE, blocked: handwritten trapping signed
// arithmetic, see decomp_state/notes/framebuf_func_001FA860.md): four
// stack-deallocation `addiu sp,sp,N` (0xE0, 0x70, 0x80, 0x90) with a nop
// after each of the first three, plus the alignment nop before
// SetupFS_AA_buffer. The unit sizes do not match the parent's frame, as in
// the documented multi-unit dead addiu-sp tail family. Nothing reaches it
// (no Ghidra function; tools/deadness_scan.py: 0 references), so the bytes
// are preserved with raw asm per the dead-tail policy; the parent keeps its
// INCLUDE_ASM.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001FA958, 0x1C\n"
    "glabel func_001FA958\n"
    "    .word 0x27BD00E0\n"
    "    .word 0x00000000\n"
    "    .word 0x27BD0070\n"
    "    .word 0x00000000\n"
    "    .word 0x27BD0080\n"
    "    .word 0x00000000\n"
    "    .word 0x27BD0090\n"
    "endlabel func_001FA958\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", SetupFS_AA_buffer__Fiiiiii);

extern s64 frameBufferColor __attribute__((section(".data")));

void SetBackgroundColor(s32 r, s32 g, s32 b) {
    frameBufferColor = (long)r | ((long)g << 8) | ((long)b << 0x10) | (0x8000ULL << 0x10);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", PutDispBuffer__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", PutDrawBufferLarge__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", framebuf_appendLargeSetup__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", PutDrawBufferSmall__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB440);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", AA_BlurPass__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", framebuf_appendSmallSetup__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB740);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB8F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FBAB0);
