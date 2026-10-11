#include "common.h"
#include "levelmem.h"

extern char text_VRAM_END[];
extern int currentVuChain;

// The level memory regions chain directly behind the boot text: InitMemSlots
// rounds the end of the resident code up to a 16K boundary for the region
// base, then places the VU chain, HUD heap, and level descriptor regions in
// sequence after it.
#define LEVEL_ARCHIVE_ALIGNMENT 0x4000U
// The level descriptor base sits this far past field_0x10.
#define LEVELMEM_LEVEL_BASE_OFFSET 0x30000
// Reserved occlusion data regions whose contents level code relocates;
// Deadlocked's InitMemSlots writes the identical triple at these addresses.
#define OCCL_POINTS_ADDR 0x7000000
#define OCCL_GRIDS_ADDR 0x7100000
#define OCCL_DEBUG_ADDR 0x7200000

void InitMemSlots(void) {
    levelMem.base = ((u32)(text_VRAM_END + LEVEL_ARCHIVE_ALIGNMENT - 1) & ~(LEVEL_ARCHIVE_ALIGNMENT - 1));
    levelMem.field_0x04 = levelMem.base;
    levelMem.field_0x08 = levelMem.base + currentVuChain;
    levelMem.field_0x0C = levelMem.field_0x08 + currentVuChain;
    levelMem.field_0x10 = levelMem.field_0x0C + LEVELMEM_HUD_HEAP_SIZE;
    levelMem.level_base = levelMem.field_0x10 + LEVELMEM_LEVEL_BASE_OFFSET;
    levelMem.level_end = levelMem.level_base;
    levelMem.occl_points = OCCL_POINTS_ADDR;
    levelMem.occl_grids = OCCL_GRIDS_ADDR;
    levelMem.debug = OCCL_DEBUG_ADDR;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/initonce", InitOnce__Fv);

// Unreachable dead tail the original compiler emitted after InitOnce__Fv
// (blocked: EGC mixed-mode store wall, see
// decomp_state/notes/initonce_InitOnce__Fv.md): a dead addiu sp,sp,0x30
// dealloc matching no live frame allocation, plus the alignment nop before
// the next object's code. Nothing reaches it (no Ghidra function;
// tools/deadness_scan.py: 0 references), so the bytes are preserved with
// raw asm per the dead-tail policy; the parent keeps its INCLUDE_ASM.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_00201A20, 0x4\n"
    "glabel func_00201A20\n"
    "    .word 0x27bd0030\n"
    "endlabel func_00201A20\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);
