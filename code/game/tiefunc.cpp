#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/tiefunc", DmaTieTextures__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/tiefunc", PatchTieGifs);

// C linkage: the GIF patcher is supplied by generated assembly at its
// original unmangled entry point.
extern "C" void PatchTieGifs(void);

// Tie texture state: D_001E3000 holds the patch count and index table that
// PatchTieGifs walks; D_001E2A00 is the 0x200-entry palette it reads. The
// D_001E4200 / D_001E3E00 buffers are the staged copies written over them.
extern u8 D_001E3000[];
extern u8 D_001E4200[];
extern u8 D_001E2A00[];
extern u8 D_001E3E00[];

void UpdateTieTextures(void) {
    PatchTieGifs();
    FastMemCopy(D_001E3000, D_001E4200, 0x200);
    FastMemCopy(D_001E2A00, D_001E3E00, 0x400);
    PatchTieGifs();
}

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
