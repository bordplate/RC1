#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/disp", setImageTag);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/disp", vblankHandler);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/disp", handler_endimage);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/disp", startDisplay__Fi);

void endDisplay(void) {
    *(int*)0x1611E0 = 0;
}
