#ifndef LEVELMEM_H
#define LEVELMEM_H

#include "types.h"

// Level memory map at 0x1940C0, written by the level loader. MemSlots
// (0x1940C4) and hudHeapBase (0x1940CC) are its 0x04/0x0C fields; 0x1C holds
// the decode buffer base used to position the movie/audio sub-buffers.
typedef struct {
    u32 field_0x00;
    u32 field_0x04;
    u32 field_0x08;
    u32 field_0x0C;
    u32 field_0x10;
    u32 field_0x14;
    u32 field_0x18;
    u32 decodeBufBase;
} LevelMem;
extern LevelMem levelMem __attribute__((section(".data")));

#endif
