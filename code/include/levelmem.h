#ifndef LEVELMEM_H
#define LEVELMEM_H

#include "types.h"

// Level memory map at 0x1940C0, laid out by InitMemSlots__Fv
// (code/game/initonce.cpp) from the 16K-aligned end of the boot text; the
// level loader then advances level_end and sets decodeBufBase, which
// positions the movie/audio sub-buffers. field_0x04 and field_0x0C are also
// aliased by the MemSlots (0x1940C4) and hudHeapBase (0x1940CC) symbols; the
// HUD heap spans LEVELMEM_HUD_HEAP_SIZE from field_0x0C. The occlusion fields
// hold reserved main-memory regions; Deadlocked's InitMemSlots writes the
// same triple under the same names, and the occlusion sampler reads
// occl_points, which is also aliased by the occlSamplePoints symbol (0x1940E0).
typedef struct {
    u32 base;
    u32 field_0x04;
    u32 field_0x08;
    u32 field_0x0C;
    u32 field_0x10;
    u32 level_base;
    u32 level_end;
    u32 decodeBufBase;
    u32 occl_points;
    u32 occl_grids;
    u32 debug;
} LevelMem;
extern LevelMem levelMem __attribute__((section(".data")));

// Size of the HUD heap region: InitMemSlots advances field_0x10 past
// field_0x0C by this amount, and Hud_HeapReset__Fv derives hudHeap.heapEnd
// from hudHeapBase with the same size.
#define LEVELMEM_HUD_HEAP_SIZE 0x64000

#endif
