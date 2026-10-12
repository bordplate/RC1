#include "common.h"
#include "types.h"
#include "hud.h"

// Unreachable multi-unit dead tail the original compiler emitted after
// OcclAppendRectsHi (0x200F90, matched in hud_post_post2_post3_tail_occl):
// three stack deallocations (0x20, 0x30, 0xF0) with alignment nops, matching
// no live frame of the parent (whose own epilogue deallocates 0x10). The
// deadness scan (tools/deadness_scan.py) finds no references and Ghidra has
// no function at or containing 0x201110. EGC 2.95.2 never regenerates dead
// frame deallocations after the epilogue (probed; see
// decomp_state/notes/989snd_func_0012E078.md), so the bytes are preserved
// with raw asm. The .align 3 and the nonmatching/glabel pair reproduce the
// generated assembly's layout.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_00201110, 0x14\n"
    "glabel func_00201110\n"
    "    .word 0x27bd0020\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0030\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd00f0\n"
    "endlabel func_00201110\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post3_tail", func_00201128);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post3_tail", func_00201200);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post3_tail", func_002012A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post3_tail", draw_bootImage__Fi);
