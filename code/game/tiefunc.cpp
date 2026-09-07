#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/tiefunc", DmaTieTextures__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/tiefunc", PatchTieGifs);

INCLUDE_ASM("code/_generated/nonmatchings/game/tiefunc", func_00235840);

extern "C" void LightTies(u16*);
extern "C" u16 D_001E3200[];
extern "C" u16 D_001E4400[];

extern "C" void func_00235898(void) {
    LightTies(D_001E3200);
    LightTies(D_001E4400);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/tiefunc", DrawTies_1);

INCLUDE_ASM("code/_generated/nonmatchings/game/tiefunc", DrawTies_2);
