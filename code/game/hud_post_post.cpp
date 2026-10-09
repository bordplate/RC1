#include "common.h"
#include "types.h"
#include "hud.h"

void Hud_SetupChannelIcon(HudChanSlot* slot, int iconId) {
    int index = Hud_GetIconIndex(iconId);
    slot->iconId = ((HudIconDef*)hudHeap.iconTable)[index].id;
    slot->iconIndex = index;
    slot->iconAnimType = ((HudIconDef*)hudHeap.iconTable)[index].animType;
    slot->iconStart = ((HudIconDef*)hudHeap.iconTable)[index].start;
}

// Unreachable dead tail after Hud_SetupChannelIcon (func_001FF500,
// 0x1FF500): one `addiu sp,sp,0x40`; the unit size does not match the
// parent's 0x20 frame (see the 989snd dead addiu-sp tail family). Nothing
// reaches 0x1FF568 (0 jal/j/branch/data references; no Ghidra function), so
// it is not a function; the original compiler emitted this byte after the
// parent's RTL, so it is preserved here as an exact word. The trailing nop
// pads to the 8-aligned func_001FF570 entry.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001FF568, 0x4\n"
    "glabel func_001FF568\n"
    "    .word 0x27bd0040\n"
    "endlabel func_001FF568\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);
