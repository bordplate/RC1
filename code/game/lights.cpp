#include "common.h"
#include "actuator.h"
#include "camera.h"
#include "lights.h"

// C linkage: the implementation remains supplied by generated assembly at the
// original unmangled entry point.
extern "C" void CreatePointLight(int idx);
void DetachPointLight(int idx);

// Symbol overrides: the originals use unmangled labels despite these C++
// source declarations.
void UpdateAllPointLights() asm("UpdateAllPointLights");
void RefreshPointLight(int idx) asm("RefreshPointLight");

// C++ linkage (mangles to FastAddRots__Fff): fast add-with-wrap in
// game/fastfunc; returns a + b wrapped into [-100.0, 100.0).
extern float FastAddRots(float a, float b);

// Rewrite the orbiting background light block (DirLights[13]) from the
// current camera radius, then walk the 8 point-light slots: a slot is
// created (state 1 -> 2) or refreshed (state 2) whenever its target has
// moved more than 8.0 from the last synced position.
//
// EGC 2.95.2 schedule/RA tiebreak (byte-verified): the loop must keep ONE
// induction anchored at updatePos (state at -0x10, the synced copy at 0, and
// the distance argument all the same register), the two array bases must be
// one-time intermediates rather than folded into the field offsets, and the
// 8.0 threshold must live in f22 so f20/f21/f22 are all saved (frame 0x90).
// Natural source forms either fold the base (reusing the DirLights s0 and
// shrinking the frame) or anchor the chain at the record base (state at +0x10,
// a separate arg register and a third tail increment). The induction, base and
// threshold registers are therefore pinned, the color_a/dir_a store order is
// chosen to land each store where the original schedules it, and a few
// memory barriers pin the branch-likely and call delay slots.
void UpdateAllPointLights() {
    register int one asm("$21") = 1;
    register int two asm("$20") = 2;
    register int i asm("$19") = 0;
    DirLights[13].color_a.w = -0.3f;
    DirLights[13].color_a.x = 0.8f;
    DirLights[13].color_a.y = 0.8f;
    DirLights[13].color_a.z = 0.8f;
    float phase = FastAddRots(currentCamera.rot.radius, -0.8f);
    register float threshold asm("$f22") = 8.0f;
    DirLights[13].dir_a.x = FastCos(phase) * 0.866f;
    float dirY = FastSin(phase) * 0.866f;

    register PointLight* plBase asm("$3") = PointLights;
    register PL_Data* dataBase asm("$2") = PointLightData;
    register LightQuad* up asm("$17") = &dataBase[0].updatePos;
    register LightQuad* pl asm("$18") = &plBase[0].pos;

    DirLights[13].dir_a.y = dirY;
    DirLights[13].dir_a.z = -0.5f;
    DirLights[13].dir_a.w = 0.0f;

    do {
        int* state = (int*)((u8*)up - 0x10);
        if (*state != 0) {
            asm volatile("" : : : "memory");
            if (threshold < FastVecDist(pl, up)) {
                *up = *pl;
                asm volatile("" : : : "memory");
                int s = *state;
                if (s == one) {
                    asm volatile("" : : : "memory");
                    CreatePointLight(i);
                    *state = two;
                } else if (s == two) {
                    asm volatile("" : : : "memory");
                    RefreshPointLight(i);
                }
            }
        }
        i++;
        up = (LightQuad*)((u8*)up + 0x30);
        pl = (LightQuad*)((u8*)pl + 0x20);
    } while (i < 8);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/lights", CreatePointLight);

void RefreshPointLight(int idx) {
    DetachPointLight(idx);
    CreatePointLight(idx);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/lights", DetachPointLight__Fi);
