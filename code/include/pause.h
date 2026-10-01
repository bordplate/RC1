#ifndef RC1_PAUSE_H
#define RC1_PAUSE_H

#include "types.h"

#define PAUSE_SPRITE_LIST_COUNT 8

// Pause menu item with a sprite-list selection: an 8-entry array of
// sprite-list pointers at offset 0x30 (element 1, at 0x34, is the list the
// select callbacks stage in and read back).
typedef struct {
    u8 pad[0x30];
    u32 spriteLists[PAUSE_SPRITE_LIST_COUNT];
} PauseSpriteListMode;

// Staging buffer for the pause item's sprite-list pointers; the card-screen
// setup (0x226B08) ships it to IOP RAM 0x70000150.
extern u32 pauseIopSpriteLists[PAUSE_SPRITE_LIST_COUNT];

#endif
