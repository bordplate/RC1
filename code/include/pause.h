#ifndef RC1_PAUSE_H
#define RC1_PAUSE_H

#include "types.h"

// Pause menu item with a sprite-list selection: the menu object carries the
// chosen sprite-index list pointer at offset 0x34 (element 1 of the 7-entry
// pointer array that begins at offset 0x30, which func_0021CA60 copies to
// D_00141EA0).
typedef struct {
    u8 pad[0x34];
    u32 spriteList;
} PauseSpriteListMode;

#endif
