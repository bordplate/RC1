#include "common.h"
#include "types.h"

// Pool of map slots. Each slot keeps a pointer to its map data (slotPtr) and
// the slot key (slotKey); findMapSlot scans the keys and reports a slot only
// when its pointer is non-zero. slotPtr and slotKey are laid out back to back,
// so the pointer backing slotKey[i] sits MAP_SLOT_COUNT words before it.
#define MAP_SLOT_COUNT 5
struct MapSlotPool {
    u8 pad[0x278];
    void* slotPtr[MAP_SLOT_COUNT];
    s32 slotKey[MAP_SLOT_COUNT];
};
extern struct MapSlotPool mapSlotPool;

INCLUDE_ASM("code/_generated/nonmatchings/game/map", func_00204CF0);

INCLUDE_ASM("code/_generated/nonmatchings/game/map", func_00204E30);

INCLUDE_ASM("code/_generated/nonmatchings/game/map", func_00204EF8);

INCLUDE_ASM("code/_generated/nonmatchings/game/map", func_00204F60);

INCLUDE_ASM("code/_generated/nonmatchings/game/map", func_00205000);

int findMapSlot(int key) {
    int count = 0;
    struct MapSlotPool* pool = &mapSlotPool;
    s32* p = pool->slotKey;
    do {
        if (p[-MAP_SLOT_COUNT] != 0 && *p == key)
            return count;
        count++;
        p++;
    } while (count < MAP_SLOT_COUNT);
    return -1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/map", func_002050E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/map", func_00205220);

INCLUDE_ASM("code/_generated/nonmatchings/game/map", func_00205278);

INCLUDE_ASM("code/_generated/nonmatchings/game/map", func_002053D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/map", func_00205440);

INCLUDE_ASM("code/_generated/nonmatchings/game/map", UNK_NoMapAvailable);

INCLUDE_ASM("code/_generated/nonmatchings/game/map", func_00206710);

INCLUDE_ASM("code/_generated/nonmatchings/game/map", func_00206860);
