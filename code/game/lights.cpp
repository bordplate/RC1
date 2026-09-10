#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/lights", UpdateAllPointLights);

INCLUDE_ASM("code/_generated/nonmatchings/game/lights", CreatePointLight);

// C linkage: the implementation remains supplied by generated assembly at the
// original unmangled entry point.
extern "C" void CreatePointLight(int idx);
void DetachPointLight(int idx);

// Symbol override: the original callback uses an unmangled label despite this
// C++ source declaration.
void RefreshPointLight(int idx) asm("RefreshPointLight");
void RefreshPointLight(int idx) {
    DetachPointLight(idx);
    CreatePointLight(idx);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/lights", DetachPointLight__Fi);
