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

INCLUDE_ASM("code/_generated/nonmatchings/game/initonce", func_00201A20);
