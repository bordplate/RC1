#include "common.h"
#include "types.h"
#include "hud.h"

// C linkage: the original symbol names of these two VU1 sprite packet
// builders are not recoverable (the boot ELF is stripped); the build links
// their unmangled Splat placeholder names (0x1FFC30 and 0x1FFE18), which is
// what the generated call sites reference.
extern "C" void func_001FFC30(int frame, int x, int y, int width, int height,
                              int alpha);
extern "C" void func_001FFE18(int frame, int x, int y, int width, int height,
                              int alpha);

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

// HUD bank icon of the three-segment message bar: frame 0 is the middle
// tile, frame 1 the end cap.
#define HUD_MSG_BAR_ICON 0x7580
// Pixel width of each fixed end cap; the middle tile takes the remainder.
#define HUD_MSG_BAR_CAP_WIDTH 0x20

// Symbol override: the stripped ELF has no name for this function; the Splat
// placeholder func_00201128 pinned at 0x201128 is the call target of its
// sole caller, the HUD message printer (0x201268).
void Hud_DrawMsgBar(int x, int y, int width, int height, int alpha)
    asm("func_00201128");

// Draws the three-segment icon bar behind the HUD message text: a 0x20-wide
// cap at x, the middle tile from x + 0x20 to x + width - 0x20, and a final
// cap at x + width - 0x20. The final cap is drawn through the 4-corner
// builder func_001FFE18 instead of the two-corner builder.
void Hud_DrawMsgBar(int x, int y, int width, int height, int alpha) {
    int frameMiddle = GetIconFrame(HUD_MSG_BAR_ICON, 0);
    int frameCap = GetIconFrame(HUD_MSG_BAR_ICON, 1);

    func_001FFC30(frameCap, x, y, HUD_MSG_BAR_CAP_WIDTH, height, alpha);
    func_001FFC30(frameMiddle, x + HUD_MSG_BAR_CAP_WIDTH, y,
                  width - 2 * HUD_MSG_BAR_CAP_WIDTH, height, alpha);
    func_001FFE18(frameCap, x + width - HUD_MSG_BAR_CAP_WIDTH, y,
                  HUD_MSG_BAR_CAP_WIDTH, height, alpha);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post3_tail", func_00201200);

// Unreachable dead tail the original compiler emitted after
// func_00201200 (blocked): two stack deallocations
// (addiu sp,+0x10; addiu sp,+0x80) matching no live frame, plus the
// alignment nop before draw_bootImage. The deadness scan
// (tools/deadness_scan.py) finds no references and Ghidra has no function
// at or containing 0x2012A8. EGC 2.95.2 never regenerates a dead frame
// deallocation after the epilogue, so the bytes are preserved with raw asm.
// The .align 3 and the nonmatching/glabel pair reproduce the generated
// assembly's layout.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_002012A8, 0xC\n"
    "glabel func_002012A8\n"
    "    .word 0x27bd0010\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0080\n"
    "endlabel func_002012A8\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_post3_tail", draw_bootImage__Fi);
