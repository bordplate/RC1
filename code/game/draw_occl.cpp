#include "common.h"
#include "types.h"
#include "camera.h"
#include "pad_state.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_occl", func_001F0CE0);

// Unreachable dead tail the original compiler emitted after
// func_001F0CE0 (occlCamDebugSampler): a 0xB0 stack deallocation matching
// no live frame, plus the alignment nop before projectWorldPoint__FPfT0.
// The deadness scan (tools/deadness_scan.py) finds no references. EGC
// 2.95.2 never regenerates a dead frame deallocation after the epilogue
// (probed; see decomp_state/notes/989snd_func_0012E078.md), so the bytes
// are preserved with raw asm. The .align 3 and the nonmatching/glabel pair
// reproduce the generated assembly's layout.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001F2068, 0x4\n"
    "glabel func_001F2068\n"
    "    .word 0x27bd00b0\n"
    "endlabel func_001F2068\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);
