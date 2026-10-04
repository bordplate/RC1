#include "common.h"
#include "types.h"
#include "pause.h"

// Per-level table of pause sprite-list pointers: 19 slots indexed by
// currentLevelId % 19; slot 18 is null.
#define PAUSE_LEVEL_SPRITE_LIST_COUNT 19
extern u32 pauseLevelSpriteLists[PAUSE_LEVEL_SPRITE_LIST_COUNT];
extern int currentLevelId;

// Symbol override: pause-menu callback in the original function-pointer
// table at 0x1D4578; stages the current level's sprite list into slot 1
// (offset 0x34) of the item's spriteLists and returns 0. The (u32) cast
// keeps the level modulo unsigned (divu), as in the original.
int pause_setLevelSpriteList(PauseSpriteListMode* mode) asm("func_00221D28");

int pause_setLevelSpriteList(PauseSpriteListMode* mode) {
    mode->spriteLists[1] =
        pauseLevelSpriteLists[(u32)currentLevelId % PAUSE_LEVEL_SPRITE_LIST_COUNT];
    return 0;
}
