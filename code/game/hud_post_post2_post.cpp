#include "common.h"
#include "types.h"
#include "hud.h"

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

// Returns the frame-table index (icon start + `frame`) of the `frame`-th
// frame of `icon`, or 0 when the icon is not in the table, the frame is out
// of range, or the frame's palette/texture ram entry is not yet loaded.
//
// Match-sensitive: the entry pointer must be an integer add (index * 8 + table)
// rather than a pointer subscript (the pointer form emits the addu with swapped
// operands), the bit-31 mask must be pinned to $8 (keeps its lui hoisted above
// the palette load and the and in the tail), and the validity checks must stay
// in the nested-ternary grouping below; each is required for byte parity.
int GetIconFrame(int icon, int frame) {
    int index = Hud_GetIconIndex(icon);
    HudIconDef* entry = (HudIconDef*)(index * sizeof(HudIconDef) + hudHeap.iconTable);
    if (entry->id != HUD_SLOT_RESET_ICON_ID && frame < entry->len) {
        int start = entry->start;
        register unsigned int mask asm("$8");
        mask = HUD_RAM_ENTRY_FLAG;
        int fi = start + frame;
        return (int)hudHeap.pals[hudHeap.frames[fi].hPal].ram >= 0
            ? ((hudHeap.texs[hudHeap.frames[fi].hTex].ram & mask) == 0
                ? fi : 0)
            : 0;
    }
    return 0;
}
