#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/effects", func_001EDC50);

INCLUDE_ASM("code/_generated/nonmatchings/game/effects", func_001EE008);

INCLUDE_ASM("code/_generated/nonmatchings/game/effects", func_001EE328);

// Unreachable dead tail the original compiler emitted after
// func_001EE338 (occlusion-debug sprite drawer, blocked): a 0xA0 stack
// deallocation matching no live frame, plus the alignment nop before
// func_001EE4B0. The deadness scan (tools/deadness_scan.py) finds no
// references and Ghidra has no function at or containing 0x1EE4A8. EGC
// 2.95.2 never regenerates a dead frame deallocation after the epilogue
// (probed; see decomp_state/notes/989snd_func_0012E078.md), so the bytes
// are preserved with raw asm. The .align 3 and the nonmatching/glabel pair
// reproduce the generated assembly's layout.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001EE4A8, 0x4\n"
    "glabel func_001EE4A8\n"
    "    .word 0x27bd00a0\n"
    "endlabel func_001EE4A8\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

INCLUDE_ASM("code/_generated/nonmatchings/game/effects", func_001EE4B0);

INCLUDE_ASM("code/_generated/nonmatchings/game/effects", func_001EE640);
