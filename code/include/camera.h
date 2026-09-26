#ifndef CAMERA_H
#define CAMERA_H

#include "types.h"

// 128-bit (16-byte) quadword. On the R5900 EGC lowers a `mode(TI)` value to
// lq/sq (128-bit) transfers; a plain 64-bit `long` lowers to ld/sd instead.
typedef unsigned int CameraQuad __attribute__((mode(TI)));

// 16-byte float4 vector; the same 16 bytes as CameraQuad but addressed as
// individual floats (lwc1/swc1) by the camera transition/blend code.
struct Vec4 {
    float x, y, z, w;
};

// Polar orientation of a camera: azimuth/elevation in radians about the
// forward/up axes and the distance from the reference point. Filled by
// Camera_Pos2Polar3d.
struct PolarSm {
    float azimuth;
    float elevation;
    float radius;
};

// 64-byte (four CameraQuad) orientation matrix. The transition helpers stage
// one on the stack and commit it with func_001FA2B8.
struct CameraMatrix {
    CameraQuad q[4];
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
    u8 pad_04[0xC];
    u32 field_10;         // 0x10: zeroed when a direct (mode 0) camera is committed
    f32 quatInterp;       // 0x14: latched copy of reqQuatInterpAdd
    f32 reqQuatInterpAdd; // 0x18: requested quaternion interp increment
    u32 field_1C;         // 0x1C: zeroed when a direct (mode 0) camera is committed
    f32 posInterp;        // 0x20: latched copy of reqPosInterpAdd
    f32 reqPosInterpAdd;  // 0x24: requested position interp increment
    u8 pad_28[8];
    CameraQuad pose1;     // 0x30: low 64 bits latched from activeCam1
    CameraQuad pose0;     // 0x40: low 64 bits latched from activeCam0
    CameraQuad activeCam0;  // 0x50
    CameraQuad activeCam1;  // 0x60
    PolarSm polar;        // 0x70: azimuth/elevation/radius filled by Camera_Pos2Polar3d
    u32 blendStep;        // 0x7C: scaled interp frame count (func_001FA6C0 arg)
    f32 blendStepInv;     // 0x80: 1.0f / blendStep float
    u32 reqInterpFrames;    // 0x84: interpolation frame count for the polar blend
    u8 pad_88[0x28];
    CameraQuad blendWork;   // 0xB0: work slot written by the per-frame transform
    CameraQuad pendingCam0; // 0xC0
    CameraQuad pendingCam1; // 0xD0
};

// 16-byte per-axis camera offset timer (Camera_OffsetTick state); two slots
// live at +0x160/+0x170 of struct Camera and drive the two orientation axes of
// the decaying position oscillation.
struct CamOffsetRec {
    float amp;      // +0x00 oscillation amplitude
    float result;   // +0x04 computed offset magnitude
    int total;      // +0x08 remaining timer frames (decremented each tick)
    int elapsed;    // +0x0C elapsed frames
};

// 0x186F40 (currentCamera) and 0x18CF10 (drawCamera); the level camera code
// indexes the per-slot UpdateCam block through pCurrentUpdCam/pLastUpdCam.
struct Camera {
    // First 4x4: the camera basis written by UpdateDrawCamera. Rows are
    // [ -q1x, -q2x, q0x, 0 ], [ -q1y, -q2y, q0y, 0 ], [ -q1z, -q2z, q0z, 0 ],
    // [ 0, 0, 0, 1 ] where q0-q2 are the staged orientation quads; entry 0
    // doubles as the perspective scale used by projectWorldPoint.
    float mtx0[16];        // 0x00
    float matrix[16];      // 0x40
    // Derived 4x4 slots maintained by UpdateDrawCamera from the camera and the
    // view context (viewCtx): mtx2 = camera x fMtx with the hvdf row blend,
    // mtx3 = camera x field_40, mtx4 = field_40 guard-scaled rows x mtx0.
    float mtx2[16];        // 0x80
    float mtx3[16];        // 0xC0
    float mtx4[16];        // 0x100
    float pos;             // 0x140: camera position x
    float posY;            // 0x144
    float posZ;            // 0x148
    char pad_14C[4];
    PolarSm rot;           // 0x150: azimuth/elevation/radius decomposed from orientMtx
    char pad_15C[4];
    CamOffsetRec offset0;  // 0x160: first offset-timer axis
    CamOffsetRec offset1;  // 0x170: second offset-timer axis
    u32 pCurrentUpdCam;    // 0x180: current UpdateCam slot (low 32 bits)
    u32 pLastUpdCam;       // 0x184: previous UpdateCam slot (low 32 bits)
    char pad_188[0x28];
    float f1B0;
    char pad_1B4[0xA4];
    float screenFade;      // 0x258: per-camera screen fade consumed from the
                           // global screenFade each frame (Camera_HandleScreenFade)
    char pad_25C[0x14];
    CamBlender blender;    // 0x270
    // 0x350: orientation matrix committed by the camera transition (identity
    // on camera reset); the occlusion setup reads it as the camera transform.
    CameraMatrix orientMtx; // 0x350
    char pad_390[8];        // 0x390
    u32 camTimer;          // 0x398
};

extern Camera currentCamera;
extern Camera drawCamera;
extern CamBlender camTransState __attribute__((section(".data")));

// 0x18CD00: view context block shared by InitViewContext, UpdateViewContext
// and UpdateDrawCamera. Field names follow the descendant layout in
// reference/dl (ViewContext); field_40 is the float 4x4 applied before fMtx
// and the source of the two guard-scaled rows, its name is not yet resolved.
struct ViewCtx {
    u32 pad_00[0x10];
    CameraMatrix field_40; // 0x40
    u32 pad_80[0x10];
    CameraMatrix fMtx;     // 0xC0
    CameraMatrix nfMtx;    // 0x100
    CameraMatrix hMtx;     // 0x140
    u32 pad_180[0x20];
    float hvdf[4];         // 0x1A0: 3-vector blended into the mtx2 row scales
    u32 pad_1B0[0x10];
    float guardX;          // 0x1C0: scale for the two field_40 rows
    u32 pad_1C4[0x3C];
};
extern ViewCtx viewCtx;

// C linkage: handwritten VU polar builders in the generated sce/lib region
// (aliases in config/linker_aliases.ld). Each rotates the 64-byte CameraMatrix
// in place (dst == src) about one polar axis; the init variant fills the 4
// quads with (0, 0, 0, 1) first.
extern "C" void camPolarInit(CameraMatrix* m);
extern "C" void camPolarRot0(CameraMatrix* dst, CameraMatrix* src, float angle);
extern "C" void camPolarRot1(CameraMatrix* dst, CameraMatrix* src, float angle);
extern "C" void camPolarRot2(CameraMatrix* dst, CameraMatrix* src, float angle);

// C linkage: handwritten VU matrix/vector helpers in the generated
// fast-function region (aliases in config/linker_aliases.ld).
// draw_transformMatrix: 64-byte 4x4 multiply, dst[i] = dot(cols[i], rows[i])
// per 16-byte row. draw_scaleQuad: full 16-byte vector scale by f12 (the
// FastVecScale variant that also scales the w component).
extern "C" void draw_transformMatrix(void* dst, void* cols, void* rows);
extern "C" void draw_scaleQuad(void* dst, void* src, float w);
// 0x13F490: 16-byte camera position offset added to staged camera quads;
// level-provided (zero in boot).
extern CameraQuad camPosOffset __attribute__((section(".data")));

// 0x18C318: occlusion camera debug-sampler state. +0x14 (occlCamStaged) is
// nonzero while the occlusion subsystem stages its own camera transform, in
// which case camera switches and occlusion-visibility setup skip committing
// to currentCamera. The remaining fields are driven by the occlusion debug
// sampler state machine (state/subState) fed by the pad button masks.
struct OcclCamState {
    u8 pad_04[4];
    u32 bits1; // +0x04: per-subState bitfield toggled by the case 2 sampler
    u32 bits0; // +0x08: per-subState bitfield toggled by the case 0 sampler
    u32 state; // +0x0C: main sampler state (0..5)
    u32 subState; // +0x10: sub sampler state (0..10)
    u32 staged; // +0x14 (occlCamStaged)
    u32 step1; // +0x18: 0..7 wrap counter (case 1 subState 1)
    u32 step2; // +0x1C: 0..2 wrap counter (case 1 subState 2, case 6)
    u32 flagA; // +0x20: toggle (case 1 subState 3)
    u32 flagB; // +0x24: toggle (case 1 subState 4)
    u32 flagC; // +0x28: toggle gating the fog params (case 1 subState 5)
    u32 stepD; // +0x2C: 0..2 wrap counter (case 6)
    u32 flagE; // +0x30: toggle (case 1 subState 7)
    u32 flagF; // +0x34: toggle gating the camera scale (case 1 subState 8)
    u32 stepG; // +0x38: -1..0x24 wrap counter (case 1 subState 9)
    u32 index; // +0x3C: 0..0x10 wrap index (case 1 subState 10)
    u8 pad_40[0x44]; // +0x40..+0x83
    f32 angleA; // +0x84: FastSubRots(occlCamVecFloatA - posZ, radius)
    f32 angleB; // +0x88: FastSubRots(FastArcTan result, radius)
    f32 angleC; // +0x8C: FastArcTan(dy, dz)
    f32 angleD; // +0x90: occlCamVecFloatA - posZ
    s32 posIntX; // +0x94: (int)pos >> 2
    s32 posIntY; // +0x98: (int)posY >> 2
    s32 posIntZ; // +0x9C: (int)posZ >> 2
    u8 pad_a0[4]; // +0xA0..+0xA3
    u32 bgColorCache; // +0xA4: flushed to occlBgColorReturn on 0x500
    u8 pad_a8[0x48]; // +0xA8..+0xEF
    u32 toggleH; // +0xF0: toggle (case 5 subState 0)
    u32 toggleI; // +0xF4: toggle (case 5 subState 1)
    u8 logFlag0; // +0xF8: 0x6e/0x79 sampler log flag
    u8 logFlag1; // +0xF9: 0x6e/0x79 sampler log flag
    u8 pad_fa[6]; // +0xFA..+0xFF
};
extern struct OcclCamState occlCamState;

#endif
