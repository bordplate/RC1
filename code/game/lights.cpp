#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/lights", UpdateAllPointLights);

INCLUDE_ASM("code/_generated/nonmatchings/game/lights", CreatePointLight);

extern "C" void CreatePointLight(int idx);
void DetachPointLight(int idx);

void RefreshPointLight(int idx) asm("RefreshPointLight");
void RefreshPointLight(int idx) {
    DetachPointLight(idx);
    CreatePointLight(idx);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/lights", DetachPointLight__Fi);
