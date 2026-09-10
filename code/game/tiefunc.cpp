#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/tiefunc", DmaTieTextures__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/tiefunc", PatchTieGifs);

INCLUDE_ASM("code/_generated/nonmatchings/game/tiefunc", func_00235840);

// C linkage: the lighting routine is supplied by generated assembly at its
// original unmangled entry point.
extern "C" void LightTies(u16*);
extern u16 tieLightData[];
extern u16 tieLightData2[];

void refreshTieLights(void) {
    LightTies(tieLightData);
    LightTies(tieLightData2);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/tiefunc", DrawTies_1);

INCLUDE_ASM("code/_generated/nonmatchings/game/tiefunc", DrawTies_2);
