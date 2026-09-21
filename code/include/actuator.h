#ifndef GAME_ACTUATOR_H
#define GAME_ACTUATOR_H

#include "types.h"

// Actuator (shock wave) control point. 16 bytes, indexed 8 deep per pad slot.
struct actuatorWave {
    s16 type;      // +0x00 wave type 0-5
    u8 side;       // +0x02 0 = left, 1 = right
    u8 scale;      // +0x03 non-zero selects the scale accumulator
    s16 delay;     // +0x04 frames before the wave starts
    s16 lifeSpan;  // +0x06 frames remaining; type cleared when it hits 0
    s16 timer;     // +0x08 cycling position
    s16 on;        // +0x0A on-phase length
    s16 off;       // +0x0C off-phase length
    u8 power;      // +0x0E peak power
    u8 minpower;   // +0x0F floor power
};

extern actuatorWave ActuatorWave[8];

#ifdef __cplusplus
// C linkage: the implementations remain supplied by generated assembly at the
// original unmangled entry points.
extern "C" float func_001FA6C0(int x); // int -> float
extern "C" int func_001FA6D0(float x); // float -> int
// C linkage: handwritten VU fast cosine in the generated fast-function region.
extern "C" float FastCos(float x);
#endif

#endif
