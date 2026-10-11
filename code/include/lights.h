#ifndef LIGHTS_H
#define LIGHTS_H

#include "types.h"
#include "mobyutil.h"

// 16-byte quadword. The point-light vectors are only ever moved as a whole
// (the VU reads them with lqc2 and the host copies them with lq/sq), so the
// mode(TI) spelling lowers to the original 128-bit transfers; a plain 16-byte
// float struct would lower to four float loads/stores instead.
typedef unsigned int LightQuad __attribute__((mode(TI)));

// 0x19C1C0: the level's dynamic point-light targets, 8 records of 0x20. The
// space VU star shader reads color (w = per-light weight) and pos (w =
// inverse attenuation scale) for every lit star.
struct PointLight {
    LightQuad color; // 0x00
    LightQuad pos;   // 0x10
};
extern PointLight PointLights[8];

// 0x19C3C0: per-slot runtime data for PointLights, 8 records of 0x30. The
// three (offset,count) pairs head the link lists CreatePointLight and
// DetachPointLight rewrite when a slot's contents change; state is 0 while
// the slot is detached, 1 pending creation and 2 once active.
struct PL_Data {
    s16 tieOffset;     // 0x00
    s16 tieCount;      // 0x02
    s16 shrubOffset;   // 0x04
    s16 shrubCount;    // 0x06
    s16 tfragOffset;   // 0x08
    s16 tfragCount;    // 0x0A
    s16* list;         // 0x0C
    int state;         // 0x10
    int pad[3];        // 0x14
    LightQuad updatePos; // 0x20 last synced copy of PointLights[i].pos
};
extern PL_Data PointLightData[8];

// 0x19BDC0: 16 direction-light parameter blocks (0x40 each), the 0x400 bytes
// directly before PointLights. The space VU star shader blends the two blocks
// picked by the low/high nibbles of each star's light field; the space scene
// init seeds block 0 from level data, the pause map screen rewrites block 6
// and UpdateAllPointLights rewrites block 13 every frame (the orbiting
// background light). Field names follow the descendant DirLight layout.
struct DirLight {
    vec4 color_a; // 0x00
    vec4 dir_a;   // 0x10
    vec4 color_b; // 0x20
    vec4 dir_b;   // 0x30
};
extern DirLight DirLights[16];

#endif
