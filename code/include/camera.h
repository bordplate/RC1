#ifndef CAMERA_H
#define CAMERA_H

#include "types.h"

// 128-bit (16-byte) quadword. On the R5900 EGC lowers a `mode(TI)` value to
// lq/sq (128-bit) transfers; a plain 64-bit `long` lowers to ld/sd instead.
typedef unsigned int CameraQuad __attribute__((mode(TI)));

// Polar orientation of a camera: azimuth/elevation in radians about the
// forward/up axes and the distance from the reference point. Filled by
// Camera_Pos2Polar3d.
struct PolarSm {
    float azimuth;
    float elevation;
    float radius;
};

// Camera blender/transition state block (0xE0 bytes), nested at +0x270 of
// struct Camera. The camTransState symbol points at currentCamera.blender.
// Holds the active camera transform (+0x50/+0x60) and the pending transform
// (+0xC0/+0xD0) the level camera code stages before a mode switch; each is a
// 16-byte quadword.
struct CamBlender {
    s16 state;            // 0x00: transition status (caller compares against 1 and 2)
    u8 type;              // 0x02: nonzero while a staged transform is pending commit
    u8 reqType;           // 0x03: requested blend type
    u8 pad_04[0x14];
    float reqQuatInterpAdd; // 0x18: requested quaternion interp increment
    u8 pad_1C[8];
    float reqPosInterpAdd;  // 0x24: requested position interp increment
    u8 pad_28[0x28];
    CameraQuad activeCam0;  // 0x50
    CameraQuad activeCam1;  // 0x60
    u8 pad_70[0x14];
    u32 reqInterpFrames;    // 0x84: interpolation frame count for the polar blend
    u8 pad_88[0x38];
    CameraQuad pendingCam0; // 0xC0
    CameraQuad pendingCam1; // 0xD0
};

// 0x186F40 (currentCamera) and 0x18CF10 (drawCamera); the level camera code
// indexes the per-slot UpdateCam block through pCurrentUpdCam/pLastUpdCam.
struct Camera {
    float f00;
    char pad_04[0x3C];
    float matrix[16];      // 0x40
    char pad_80[0xC0];
    float pos;             // 0x140: camera position x
    float posY;            // 0x144
    float posZ;            // 0x148
    char pad_14C[0x34];
    u32 pCurrentUpdCam;    // 0x180: current UpdateCam slot (low 32 bits)
    u32 pLastUpdCam;       // 0x184: previous UpdateCam slot (low 32 bits)
    char pad_188[0x28];
    float f1B0;
    char pad_1B4[0xBC];
    CamBlender blender;    // 0x270
    char pad_350[0x48];
    u32 camTimer;          // 0x398
};

extern Camera currentCamera;
extern Camera drawCamera;
extern CamBlender camTransState __attribute__((section(".data")));
// 0x13F490: 16-byte camera position offset added to staged camera quads;
// level-provided (zero in boot).
extern CameraQuad camPosOffset __attribute__((section(".data")));

#endif
