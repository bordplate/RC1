#include "common.h"
#include "types.h"
#include "camera.h"
#include "mobyutil.h"
#include "actuator.h"

extern u8 backupCam[];
extern u8 backupCamData[];
extern u32 curCam __attribute__((section(".data")));

// 0x189650: per-camera control working area; the source buffer sits at
// camControlWork - 0x280 (0x1893D0), which backs up into backupCamData.
extern u8 camControlWork[] __attribute__((section(".data")));

// 0x20-byte import-camera entry; pVar points at per-level import data whose
// +0x1D byte selects the switch blend behavior.
struct ImportCamera {
    float pos[3];
    int type;
    float rot[3];
    u32 pVar;
};
// Level-provided pointer to the import-camera table (0 in boot).
extern ImportCamera* importCameraTable __attribute__((section(".data")));
// 0x15ED84: current level id; the polar/pos blend falls back to 0.01f on level 1.
extern int currentLevelId __attribute__((section(".data")));
// 0x1FA6D0 (fastfunc): truncates its float argument toward zero. C linkage.
extern "C" int func_001FA6D0(float x);

// 0x15F43C: global screen fade value; draw renders it as the black
// full-screen overlay (alpha = value * 128, clamped at 1.0). The original
// Camera_HandleScreenFade mixes address modes on this word: the load and the
// consume store are GP-relative, while the final zero store is a self-based
// absolute pair. In this SN-assembler TU the plain symbol expands absolute
// (unseeded single-pass) and the seeded alias (config/linker_aliases.ld)
// expands GPREL, so both names map to 0x15F43C.
extern float screenFade;
extern float screenFadeGp;
asm(".extern screenFadeGp, 4");

// C linkage: this entry point is referenced by the original unmangled camera API.
extern "C" void BackupCurrentCam(void) {
    u8* dst = backupCam;
    FastMemCopy(dst, (void*)curCam, 0xA0);
    u8* p = backupCamData;
    FastMemCopy(p, p - 0x500, 0x280);
    *(void**)(dst + 0x70) = p;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/camera", ExecuteCamPostUpdFuncs);

// Unreachable dead tail of ExecuteCamPostUpdFuncs (0x1EBCF0): two
// `move v0,zero` and one `sw zero,0(a0)` after the parent's `jr ra`.
// Nothing reaches 0x1EBD60 (0 jal/j/branch/data references; no Ghidra
// function), so it is not a function; no C form reproduces the parent plus
// this tail. The original compiler emitted these bytes after the epilogue,
// so they are preserved here as exact words. The glabel/nonmatching pair
// keeps the Splat .ld symbol pins at 0x1EBD60; the trailing nop pads to the
// 8-aligned Cam_InterpValues__FffPffff entry at 0x1EBD78.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001EBD60, 0x14\n"
    "glabel func_001EBD60\n"
    "    .word 0x0000102d\n"
    "    .word 0x00000000\n"
    "    .word 0x0000102d\n"
    "    .word 0x00000000\n"
    "    .word 0xac800000\n"
    "endlabel func_001EBD60\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

// Exponential approach: moves *offset toward the gap (target - current) by
// step * gap - decay * *offset each call, clamps it to the +/-limit band
// (when limit is nonzero) and to the gap magnitude |gap| = FastAbsF(gap).
// The original compiler clobbers the -|gap| value in a branch delay-slot
// reload before the lower-clamp store, which therefore writes -*offset
// instead of -|gap|. Returns current + *offset.
float Cam_InterpValues(float current, float target, float* offset, float step, float decay, float limit) {
    float delta = target - current;
    float v = *offset + (step * delta - decay * *offset);
    *offset = v;
    if (limit != 0.0f) {
        if (v > limit)
            *offset = limit;
        else if (v < -limit)
            *offset = -limit;
    }
    float a = FastAbsF(delta);
    if (a < *offset) {
        *offset = FastAbsF(delta);
    } else {
        a = FastAbsF(delta);
        if (*offset < -a)
            *offset = -FastAbsF(delta);
    }
    return current + *offset;
}

// Unreachable dead tail of Cam_InterpValues (0x1EBD78): a single
// `addiu sp,sp,0x50` after the parent's `jr ra; addiu sp,sp,0x30`.
// Nothing reaches 0x1EBE60 (0 jal/j/branch/data references; no Ghidra
// function), so it is not a function; no C form reproduces the parent plus
// this tail. The original compiler emitted these bytes after the epilogue,
// so they are preserved here as exact words. The glabel/nonmatching pair
// keeps the Splat .ld symbol pins at 0x1EBE60; the trailing nop pads to the
// 8-aligned Camera_handleCollWithHero__FiP9UpdateCam entry at 0x1EBE68.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001EBE60, 0x4\n"
    "glabel func_001EBE60\n"
    "    .word 0x27BD0050\n"
    "endlabel func_001EBE60\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

struct MobyInstance;
struct vec4;

// 16-bit camera mode read from the camera entry passed in a0 (the first
// parameter is declared int only so the symbol mangles to (int, UpdateCam*);
// it holds an UpdateCam* at runtime). Mode 0 keeps the hero-collision moby
// spawned, any other value clears it. Deadlocked reads the equivalent camera
// `type` at this spot, 6 bytes before UpdateCam::funcIdx.
#define CAM_COLL_MODE_OFF 0x86
// Bytes from the collision-state block back to the camera position (Camera)
// handed to the moby spawn.
#define CAM_POS_BACK_OFF 0x50

// Per-level camera data block at 0x13F350 (0x2310 bytes, all zero in boot).
// The space transition setup zero-clears the whole block
// (func_001E9B10: FastMemSet(levelCamData, 0, 0x2310)); level overlays supply
// their own copy at the same address. camPosOffset (0x13F490) is the block
// field at +0x140 (inner +0x120). The compiler materializes f98/pCollMoby/fA8
// through the inner block base (levelCamData+0x20), so inner is a distinct
// sub-block in the source.
struct LevelCamInner {
    char pad_00[0x60];    // +0x20
    f32 dir80[4];         // +0x80 direction; dir80[2] (+0x88) seeds lastMobyZ
    char pad_70[8];       // +0x90
    f32 f98;              // +0x98 pushed into camCollState.ring[4] each frame
    char pad_7C[0x1F4];   // +0x9C
    f32 dir290[4];        // +0x290 (0x13F5E0) source of camCollState.aCur
    char pad_280[0x50];   // +0x2A0
    f32 waterHeight;      // +0x2F0 water-surface z; land flag when camera is above it
    char pad_2D4[0x8];    // +0x2F4
    u32 pCollMoby;        // +0x2FC level-placed collision moby (32-bit slot)
    char pad_2E0[0xFE4];  // +0x300
    u8 hotSpotWater;      // +0x12E4 hotspot select: water (clears collMode)
    u8 hotSpotLava;       // +0x12E5 hotspot select: lava
    u8 hotSpotQuickSand;  // +0x12E6 hotspot select: quicksand
    char pad_12C7[4];     // +0x12E7
    u8 hotSpotDeathSand;  // +0x12EB hotspot select: death sand
    u8 hotSpotIceWater;   // +0x12EC hotspot select: ice water
    char pad_12CD[0xD97]; // +0x12ED
    int heroState;        // +0x2084 (0x1413D4) hero state mirrored for camera collision
    char pad_2068[4];     // +0x2088
    u32 heroStateType;    // +0x208C hero state type mirrored for camera collision
    char pad_2070[0x1F4]; // +0x2090
    int i2284;            // +0x2284 (0x1415D4)
    char pad_2268[0x88];  // +0x2288
};
struct LevelCamData {
    char pad_00[0x20];
    LevelCamInner inner;  // +0x20, 0x22F0 bytes
};
extern LevelCamData levelCamData __attribute__((section(".data")));

// Camera-collision state block at 0x1870D0; the current collision moby
// (GameCamera, 0x187194) sits at +0xC4. Moby pointers live below 0x10000000
// so the block stores/compares them as 32-bit values. The spawn reuses the
// single hoisted base register, so its position argument is a base offset.
// Updated every frame by Camera_updateCollState from levelCamData: a
// smoothed collision direction/position set, a 5-slot history ring for
// levelCamData.f98, and per-frame tracking of the level's collision moby.
struct CamCollState {
    float f00;             // +0x00 levelCamData.dir80[0]
    float f04;             // +0x04 levelCamData.dir80[1]
    float f08;             // +0x08 smoothed toward levelCamData.dir80[2]
    float f0C;             // +0x0C levelCamData.dir80[2]
    float off10;           // +0x10 Cam_InterpValues offset state for f08
    char pad_14[0xC];      // +0x14
    Vec4 dir20;            // +0x20 collision direction vector
    CameraQuad aCur;       // +0x30 current normalized level direction
    CameraQuad aPrev;      // +0x40 previous frame's aCur
    float off50;           // +0x50 Cam_InterpValues offset state for dir20.x
    float off54;           // +0x54 ... dir20.y
    float off58;           // +0x58 ... dir20.z
    char pad_5C[4];        // +0x5C
    CameraQuad v60;        // +0x60 copy of levelCamData.dir80 (last frame's)
    CameraQuad v70;        // +0x70 dir80 - v60, then overwritten by normalize(a)
    CameraQuad v80;        // +0x80 v70 - b, normalized in place
    CameraQuad b;          // +0x90 normalize(a) scaled by |dir80 - v60|
    float fA0;             // +0xA0 |dir80 - v60|
    float fA4;             // +0xA4 pre-normalization length of v80
    float fA8;             // +0xA8 |dir80 - v60| (duplicate of fA0)
    float ring[5];         // +0xAC history of levelCamData.f98, shifted per frame
    int collMode;          // +0xC0 selected hotspot mode (CAM_HOTSPOT_*, set by
                           //     Camera_updateCollMode)
    MobyInstance* pCamColl; // +0xC4 -> 0x187194
    char pad_CC[12];       // +0xC8
    u32 pCollMoby;         // +0xD4 level's collision moby (32-bit slot)
    float lastMobyZ;       // +0xD8 last observed pCollMoby pos.z
    float mobyZDelta;      // +0xDC per-frame pos.z delta
};
extern CamCollState camCollState __attribute__((section(".data")));
extern int camCollFlag;
extern int camCollFlagPrev;
extern int camCollMode;

struct UpdateCam;
// func_001E9448 is a boot-ELF stub (jr $ra) for the hero-collision moby
// spawn. Its real symbol and linkage are unknown (the body is supplied by a
// level overlay), so pin the unmangled Splat placeholder with a symbol
// override rather than assuming C linkage.
MobyInstance* func_001E9448(vec4* pos) asm("func_001E9448");
// The moby-deletion entry point at 0x20C828 is the unmangled symbol DeleteMoby
// in the boot ELF; a C++ free function here would mangle differently, so pin
// it with a symbol override instead of assuming C linkage.
void DeleteMoby(MobyInstance* moby) asm("DeleteMoby");

void Camera_handleCollWithHero(int camPtr, UpdateCam* pCam) {
    CamCollState* base = &camCollState;
    s16 mode = *(s16*)((long)camPtr + CAM_COLL_MODE_OFF);
    if (mode == 0) {
        if (base->pCamColl == 0) {
            base->pCamColl = func_001E9448((vec4*)((char*)base - CAM_POS_BACK_OFF));
        }
    } else if (base->pCamColl != 0) {
        DeleteMoby(base->pCamColl);
        base->pCamColl = 0;
    }
}

// 12-byte camera activation block at UpdateCam+0x78; the blend/activation
// fields match Deadlocked's CameraControlActivation, minus its leading
// activationType.
struct CameraControlActivation {
    float blendSpeed;   // 0x78
    u8 priority;        // 0x7C: unsigned; compared with sltu
    u8 activate;        // 0x7D: nonzero while the camera may activate
    s16 deactivate;     // 0x7E: switch dispatch value (1-6)
    s16 repCam;         // 0x80: unconfirmed, Deadlocked name
    s16 orgCam;         // 0x82: unconfirmed, Deadlocked name
};

// 0xA0-byte camera state block (see UpdateAllCameras__Fi iteration and
// BackupCurrentCam); per-level behavior is selected through lvlCamVtbl.
struct UpdateCam {
    CameraQuad mtx0;    // 0x00
    CameraQuad mtx1;    // 0x10
    CameraQuad mtx2;    // 0x20
    Vec4 posQuad;       // 0x30: vec4 position (addressed as floats)
    char pad_40[0x24];  // 0x40..0x64: rot/polar data, layout unconfirmed
    float lPos[3];      // 0x64
    u32 control;        // 0x70: camera control data (low 32 bits)
    int activationType; // 0x74: switch dispatch value (0-7)
    CameraControlActivation activation; // 0x78
    s16 importCameraIdx; // 0x84
    s16 collMode;        // 0x86
    s16 pad_88[2];
    s16 funcIdx;         // 0x8C
    s16 active;          // 0x8E
    char pad_90[0x10];
};

// One lvl.camvtbl entry (0x14 bytes), indexed by UpdateCam::funcIdx; each
// level overlay supplies its own table at the same address.
struct UpdateCamVtbl {
    int field_0x00;
    int (*activationCheck)(UpdateCam*, UpdateCam*);
    void (*runSetupToNewCam)(UpdateCam*);
    void (*collWithHero)(UpdateCam*, float);
    void (*exit)(UpdateCam*);
};
extern UpdateCamVtbl lvlCamVtbl[];

void Camera_runSetupToNewCam(UpdateCam* cam) {
    void (*fn)(UpdateCam*) = lvlCamVtbl[cam->funcIdx].runSetupToNewCam;
    if (fn)
        fn(cam);
}

// Stages the switch to pNewCam: dispatches on the current camera's deactivate
// value and the import-camera flag, optionally copies the 0x40-byte transform,
// retargets currentCamera's UpdateCam slots and control buffers, and updates the
// blender state. Deadlocked names this Camera_switchToNewCam (RC1 predates its
// trailing bool parameter).
// BLOCKED on the EGC 2.95.2 register allocator: the verified C form (see
// decomp_state/notes/camera_func_001EBF10.md) matches the semantics, frame
// (0x70), and currentCamera hi+%lo addressing, but EGC permutes the s-reg
// assignment (hi cam in s4 vs the original s1; pCurCam/pNewCam+0x30 swapped)
// and schedules the D-path `deactivate<=5` as slti+bnel instead of beql,
// leaving a 16-instruction gap. Kept as assembly until a matching form is found.
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EBF10);

// Camera priority/activation arbitration (0x1EC210). Dispatches the per-level
// activationCheck callback, then switches on pCam->activationType (0x74) to
// compare camera priorities, the import camera's cuboid, and the hero's
// grind-path state. Kept as assembly: EGC emits the 8-case switch jump table in
// an orphan camera.o(.rodata) that the Splat split has no home for (the original
// table is static data at 0x1E7730 in .data), and the body's register allocation
// and CFG scheduling do not match. See
// decomp_state/notes/camera_Camera_ActivationCheckPriority.md.
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", Camera_ActivationCheckPriority);

void Camera_Exit(UpdateCam* cam) {
    void (*fn)(UpdateCam*) = lvlCamVtbl[cam->funcIdx].exit;
    if (fn)
        fn(cam);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/camera", UpdateAllCameras__Fi);

// C linkage: unmangled entry point (Splat placeholder func_00214890) defined
// as an INCLUDE_ASM in mobyutil.cpp; the call target is that exact symbol.
extern "C" void func_00214890(float ang, vec4* dest, vec4* fwd, vec4* up);

// Converts the camera position relative to center into a polar orientation in
// res. The forward/side-projected angle is PIHALF - asin(dot/length) (length
// clamped away from zero), signed by the side dot, and written to azimuth;
// func_00214890 rebuilds a vector from that azimuth, whose up-projection feeds
// elevation (signed by the up dot) and whose full length feeds radius.
// Deadlocked twin Camera_Pos2Polar3d.
//
// EGC 2.95.2 float-register allocation: the PIHALF constant must live in $f21
// (so 0.0 falls to $f22 and the second asin subtraction folds in-place into
// $f21), yet its load stays late (right before the first asin) instead of
// hoisting into the prologue. An UNINITIALIZED $f21 register variable reserves
// the FPR without forcing an early load; the zero-byte "+f" barrier keeps the
// second subtraction in-place in $f21, and the operand-free barrier after the
// final dot stops `ang = -pihalf` from hoisting across it.
void Camera_Pos2Polar3d(PolarSm* res, vec4* pos, vec4* center,
                        vec4* fwd, vec4* side, vec4* up) {
    vec4 v0, upn, v1, v1n, tmp;
    float r, r1, len1, ang, d1, r2, len0, d2;
    // Uninitialized on purpose: reserves $f21 for PIHALF without hoisting load.
    register float pihalf asm("$f21");

    FastVecSub(&v0, pos, center);
    r = FastVecDot(&v0, up);
    FastVecNormalize(&upn, up, r);
    FastVecSub(&v1, &v0, &upn);
    r1 = FastVecDot(fwd, &v1);
    len1 = FastVecLength(&v1);
    if (len1 == 0.0f)
        len1 = 0.0001f;
    pihalf = 1.5707964f;
    ang = pihalf - FastArcSin(r1 / len1);
    FastVecNormalize(&v1n, &v1, 1.0f);
    d1 = FastVecDot(side, &v1n);
    if (d1 < 0.0f)
        ang = -ang;
    res->azimuth = ang;
    func_00214890(ang, &tmp, fwd, up);
    r2 = FastVecDot(&tmp, &v0);
    len0 = FastVecLength(&v0);
    if (len0 == 0.0f)
        len0 = 0.0001f;
    pihalf -= FastArcSin(r2 / len0);
    // Keep the subtraction in-place in $f21 (t overwrites PIHALF).
    asm volatile("" : "+f"(pihalf));
    FastVecNormalize(&v1n, &v0, 1.0f);
    d2 = FastVecDot(up, &v1n);
    // Stop `ang = -pihalf` below from hoisting across the dot.
    asm volatile("");
    ang = -pihalf;
    if (d2 < 0.0f)
        ang = pihalf;
    res->elevation = ang;
    res->radius = FastVecLength(&v0);
}
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EC710);
// Stages the pending camera transform before a mode switch: copies the
// active 16-byte quads (+0x50/+0x60) to the pending slots (+0xC0/+0xD0),
// adding camPosOffset to the first pending quad when the requested blend
// type is 2.
//
// EGC emits copy1 as lq v0/sq v0(a1) and the `reqType == 2` test as
// lbu a0 + li v0 scheduled after the sq with `bne a0,v0`. Natural forms put
// the li in the lq/sq gap and the lbu into v1; pinning src0 to $3, val to
// $2, reqType to $4 and the constant to $2 (disjoint live ranges), with a
// tied barrier between the lbu and the li, reproduces the original exactly.
void Camera_stagePendingTransform(void) {
    if (camTransState.type != 0)
        return;
    CameraQuad* dst0 = &camTransState.pendingCam0;
    asm volatile("" : "+r"(dst0));
    register CameraQuad* src0 asm("$3") = &camTransState.activeCam0;
    asm volatile("" : "+r"(src0));
    register CameraQuad val asm("$2") = *src0;
    *dst0 = val;
    register int reqType asm("$4") = camTransState.reqType;
    asm volatile("" : "+r"(reqType));
    register int two asm("$2") = 2;
    if (reqType == two)
        FastVecAdd((void*)dst0, (void*)&camPosOffset, (void*)dst0);
    CameraQuad* src1 = &camTransState.activeCam1;
    asm volatile("" : "+r"(src1));
    CameraQuad* dst1 = &camTransState.pendingCam1;
    asm volatile("" : "+r"(dst1));
    *dst1 = *src1;
}
// Commits the staged camera transform when a mode switch is pending: copies
// the 16-byte pending quads (+0xC0/+0xD0) over the active ones (+0x50/+0x60).
//
// The two 16-byte copies must emit lq/sq with every field address materialized
// in its own addiu and the first addiu parked in the beqz delay slot. EGC lowers
// any natural C form (mode(TI) field copy, pointer deref, volatile, flags) to
// ld/sd with folded offsets and a nop delay slot, so the exact original bytes are
// reproduced with a noreorder inline-asm block, as in INCLUDE_ASM.
void Camera_commitPendingTransform(void) {
    asm volatile(
        ".set noat\n\t"
        ".set noreorder\n\t"
        "lui   $2, %%hi(camTransState)\n\t"
        "addiu $6, $2, %%lo(camTransState)\n\t"
        "lbu   $3, 2($6)\n\t"
        "beq   $3, $0, 1f\n\t"
        "addiu $4, $6, 80\n\t"
        "addiu $3, $6, 192\n\t"
        "lq    $2, 0($3)\n\t"
        "sq    $2, 0($4)\n\t"
        "addiu $5, $6, 208\n\t"
        "addiu $3, $6, 96\n\t"
        "lq    $2, 0($5)\n\t"
        "sq    $2, 0($3)\n\t"
        "1:\n\t"
        ".set reorder\n\t"
        ".set at\n\t"
        : : : "memory", "2", "3", "4", "5", "6"
    );
}
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EC8A0);
// 0x50-byte stack workspace for the transition step: quat at sp+0x00 (the
// 128-bit quad staged from the target camera), mat at sp+0x10 (orientation).
// The EE vector callees are passed sp and sp+0x10 as work buffers.
struct CamTransitionWork {
    CameraQuad quat;  // sp+0x00
    CameraMatrix mat; // sp+0x10
};
// Sub-view of CamBlender at +0x10: the transition-progress scalars and the
// pose/active-cam quads viewed as float vectors. The caller passes
// camTransState + 0x10.
struct CamBlendStep {
    f32 field_10;         // +0x00 (CamBlender.field_10): quat progress 0..1
    f32 quatInterp;       // +0x04 (CamBlender.quatInterp)
    f32 reqQuatInterpAdd; // +0x08
    f32 field_1C;         // +0x0C (CamBlender.field_1C): pos progress 0..1
    f32 posInterp;        // +0x10 (CamBlender.posInterp)
    f32 reqPosInterpAdd;  // +0x14
    u8 pad_18[8];
    Vec4 pose1;           // +0x20 (CamBlender.pose1)
    Vec4 pose0;           // +0x30 (CamBlender.pose0)
    Vec4 activeCam0;      // +0x40 (CamBlender.activeCam0)
    Vec4 activeCam1;      // +0x50 (CamBlender.activeCam1)
};
// Sub-view of CamBlender at +0x70: the polar-blend parameters stepped by the
// per-frame polar transition (func_001ECCD8). The caller passes
// camTransState + 0x70.
struct CamBlendPolar {
    PolarSm polar;         // +0x00 (CamBlender.polar): azimuth/elevation/radius
    u32 blendStep;         // +0x0C (CamBlender.blendStep)
    f32 blendStepInv;      // +0x10 (CamBlender.blendStepInv)
    u32 reqInterpFrames;   // +0x14 (CamBlender.reqInterpFrames)
    u8 pad_18[8];          // +0x18 (CamBlender.pad_88 prefix)
};
// 0x18C318: occlusion camera state. +0x14 (occlCamStaged, 0x18C32C) is
// nonzero while the occlusion subsystem stages its own camera transform, in
// which case camera switches and occlusion-visibility setup skip committing
// to currentCamera. The original loads the base into s3 and reads +0x14.
struct OcclCamState {
    u8 pad_14[0x14];
    u32 staged; // +0x14 (occlCamStaged)
};
// 0x15ED60: per-frame transition step scale; the first word of a nine-word
// parameter table (0x15ED60-0x15ED80) that func_00214970 writes at level
// start, keyed on videoModePal (1.0f for NTSC, 1.1f for PAL). Scales the
// camera transition progress increments and space-transition alpha ramps.
extern f32 transStepScale;
extern struct OcclCamState occlCamState;

extern "C" float func_002133D0(float a, float b, float t);
extern "C" void func_002144D8(CameraQuad* dst, UpdateCam* src);
extern "C" void func_001FA400(float factor, Vec4* p50, Vec4* p20, CameraQuad* sp);
extern "C" void func_001FA4F8(Vec4* p50, CameraMatrix* buf);
extern "C" void func_001FA2B8(CameraMatrix* dst, const CameraMatrix* src);

// Per-frame camera transition step: lerps the active position quad toward the
// target by smoothstep(pos progress), updates orientation via the EE vector
// helpers, and advances the two clamped progress scalars. Returns 1 when both
// progress scalars have reached 1.0 (transition complete).
int Camera_TransitionStep(UpdateCam* pTarget, CamBlendStep* pB) asm("func_001ECAF8");
// Per-frame polar camera transition step: advances the polar blend of
// camTransState (azimuth/elevation/radius) toward the target camera and
// commits the staged result. Returns 1 when the blend is complete. The boot
// ELF is stripped, so the entry point is the unmangled Splat placeholder
// func_001ECCD8 (still INCLUDE_ASM); pin it with a symbol override rather
// than assuming C linkage.
int func_001ECCD8(UpdateCam* pTarget, CamBlendPolar* pB) asm("func_001ECCD8");

int Camera_TransitionStep(UpdateCam* pTarget_, CamBlendStep* pB_) {
    register UpdateCam* pTarget asm("$16") = pTarget_;
    CamBlendStep* pB = pB_;

    if (pB->field_1C == 1.0f && pB->field_10 == 1.0f)
        return 1;

    f32 factor = func_002133D0(0.0f, 1.0f, pB->field_1C);
    FastVecAdd((void*)&pB->pose0, (void*)&camPosOffset, (void*)&pB->pose0);

    pB->activeCam0.x = pB->pose0.x + (pTarget->posQuad.x - pB->pose0.x) * factor;
    pB->activeCam0.y = pB->pose0.y + (pTarget->posQuad.y - pB->pose0.y) * factor;
    pB->activeCam0.z = pB->pose0.z + (pTarget->posQuad.z - pB->pose0.z) * factor;

    if (occlCamState.staged == 0) {
        register CameraQuad* dst asm("$3") = (CameraQuad*)&currentCamera.pos;
        register CameraQuad* src asm("$4") = (CameraQuad*)&pB->activeCam0;
        asm volatile("" : "+r"(dst), "+r"(src));
        register CameraQuad value asm("$2") = *src;
        *dst = value;
    }

    CamTransitionWork work;
    func_002144D8(&work.quat, pTarget);
    f32 f2 = func_002133D0(0.0f, 1.0f, pB->field_10);
    func_001FA400(f2, &pB->activeCam1, &pB->pose1, &work.quat);
    func_001FA4F8(&pB->activeCam1, &work.mat);
    register f32 scale asm("$f0") = transStepScale;
    if (occlCamState.staged == 0) {
        func_001FA2B8(&currentCamera.orientMtx, &work.mat);
        scale = transStepScale;
    }
    f32 nc = pB->field_1C + pB->posInterp * scale;
    pB->field_1C = nc;
    if (1.0f < nc)
        pB->field_1C = 1.0f;
    scale = transStepScale;
    f32 nn = pB->field_10 + pB->quatInterp * scale;
    pB->field_10 = nn;
    if (1.0f < nn)
        pB->field_10 = 1.0f;
    return 0;
}
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ECCD8);
// Bytes from the orientMtx base (0x187290) down to currentCamera.pos
// (0x187080); the original computes the fourth copy destination with
// `addiu -0x210` from the base register, so it must stay a runtime-relative
// offset.
#define CAM_POS_BEHIND_ORIENT_OFF 0x210
// Per-frame camera blend dispatch (Camera_BlendCams in Deadlocked): steps the
// active camera toward the target through the pos/quat transition
// (Camera_TransitionStep) when the blend type is 0, otherwise through the
// polar transition (func_001ECCD8). When a step reports the blend complete
// and the occlusion subsystem has not staged its own transform
// (occlCamState.staged), commits the target's orientation matrix quads to
// currentCamera.orientMtx and the target's position quad to
// currentCamera.pos, then clears the blend state. The boot-ELF entry point is
// the unmangled Splat placeholder (the binary is stripped), so pin it with a
// symbol override instead of the cfront-mangled
// Camera_BlendCams__FP9UpdateCam.
void Camera_BlendCams(UpdateCam* pTarget) asm("func_001ED2B0");

void Camera_BlendCams(UpdateCam* pTarget) {
    register UpdateCam* pTgt asm("$16") = pTarget;
    CamBlender* pBlend = &camTransState;
    int done;
    if (pBlend->type == 0)
        done = Camera_TransitionStep(pTgt, (CamBlendStep*)&pBlend->field_10);
    else
    {
        // Force EGC to materialize pTgt into a0 for the polar call
        // (`move a0,s0`); without the tied barrier it reuses the incoming
        // a0 on both paths.
        asm volatile("" : "+r"(pTgt));
        done = func_001ECCD8(pTgt, (CamBlendPolar*)&pBlend->polar);
    }
    if (done != 0) {
        if (occlCamState.staged == 0) {
            register CameraQuad* d0 asm("$3") = (CameraQuad*)&currentCamera.orientMtx;
            asm volatile("" : "+r"(d0));
            register CameraQuad value asm("$2") = pTgt->mtx0;
            *d0 = value;
            asm volatile("");
            CameraQuad* d1 = d0 + 1;
            asm volatile("" : "+r"(d1));
            CameraQuad* s1 = (CameraQuad*)pTgt + 1;
            asm volatile("" : "+r"(s1));
            value = *s1;
            *d1 = value;
            asm volatile("");
            register CameraQuad* d2 asm("$6") = d0 + 2;
            asm volatile("" : "+r"(d2));
            CameraQuad* s2 = (CameraQuad*)pTgt + 2;
            asm volatile("" : "+r"(s2));
            value = *s2;
            *d2 = value;
            asm volatile("");
            // The fourth slot is currentCamera.pos; keep the relative form
            // (CAM_POS_BEHIND_ORIENT_OFF below the orientMtx base) rather
            // than the address so EGC keeps the original `addiu -0x210`.
            CameraQuad* d3 = (CameraQuad*)((u8*)d0 - CAM_POS_BEHIND_ORIENT_OFF);
            asm volatile("" : "+r"(d3));
            register CameraQuad* s3 asm("$4") = (CameraQuad*)pTgt + 3;
            asm volatile("" : "+r"(s3));
            value = *s3;
            *d3 = value;
        }
        // EGC duplicates the last of the two merge stores into the staged
        // branch's delay slot and points the branch at the first; the
        // original (type=0 in the delay slot and after the copy, state=0 at
        // the merge) needs this state/type source order.
        pBlend->state = 0;
        pBlend->type = 0;
    }
}
// 16-byte per-axis camera offset timer. Two slots live in currentCamera's pad
// region (0x1870A0/0x1870B0) and are passed in by the caller; each drives one
// orientation axis of the decaying position oscillation applied below.
struct CamOffsetRec {
    float amp;      // +0x00 oscillation amplitude
    float result;   // +0x04 computed offset magnitude
    int total;      // +0x08 remaining timer frames (decremented each tick)
    int elapsed;    // +0x0C elapsed frames
};

// Advances one axis of the decaying position oscillation: decrements the offset
// timer, scales the selected orientation axis (q[2] for which=0, q[0] for
// which=1) by amp*cos(2*total)*ratio^2, and adds the result to the camera
// position. Colliding with the hero in mode 6 cancels the oscillation.
void Camera_OffsetTick(CamOffsetRec* p, int which) asm("func_001ED360");

void Camera_OffsetTick(CamOffsetRec* p, int which) {
    if (curCam != 0 && ((UpdateCam*)curCam)->collMode == 6) {
        p->total = 0;
        p->elapsed = 0;
        return;
    }
    if (p->total != 0) {
        if (p->elapsed < p->total)
            p->elapsed = p->total;
        FastDecTimer(p->total);
        float ratio = func_001FA6C0(p->total) / func_001FA6C0(p->elapsed);
        float totalF = func_001FA6C0(p->total);
        float ang = FastNormalizeAngle(totalF + totalF);
        float c = FastCos(ang);
        float v = p->amp;
        v *= c;
        v *= ratio;
        float r = v * ratio;
        p->result = r;
        Vec4 vec;
        if (which == 0)
            FastVecNormalize(&vec, &currentCamera.orientMtx.q[2], r);
        else
            FastVecNormalize(&vec, &currentCamera.orientMtx.q[0], r);
        FastVecAdd(&currentCamera.pos, &currentCamera.pos, &vec);
    } else {
        p->elapsed = 0;
    }
}
// Per-frame camera-collision update from levelCamData. Normalizes
// levelCamData.dir290 (scaled by -1.0f) into a stack vec, shifts it into the
// aCur/aPrev pair, and smooths the collision direction (camCollState.dir20)
// toward it with Cam_InterpValues. Re-normalizes levelCamData.dir80 in place
// and derives the v60/v70/v80/b chain (v70 = dir80 - v60; b = normalized a
// scaled by dot(v70, a); v80 = v70 - b, normalized in place), caching the
// intermediate dot and length. Maintains the 5-slot f98 history ring, writes
// levelCamData.dir80 components into f00/f04 (and smooths f08 toward
// dir80[3] via Cam_InterpValues unless i2284 == 0x50 with heroState != 0x11), and
// tracks levelCamData.pCollMoby's oClass/pos.z when it is set.
//
// BLOCKED (EGC 2.95.2 scheduling): a C form reproduces the original register
// structure (s5=camCollState, s7=%hi, s6=&levelCamData.inner.dir290, 208-byte
// frame) but the prologue save/interleave order (f20 hoisted into the first
// jal delay slot), the materialized aCur/aPrev address registers, the
// count-up ring loop (a0 moving, a3 hi), and the s1=&t / s0=&v80 allocation
// do not match. See decomp_state/notes/camera_func_001ED470.md.
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ED470);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ED7F0);
// Camera-collision flags and hotspot modes written every frame by
// Camera_updateCollMode; nothing in the boot ELF reads the three globals
// (level overlays do). Deadlocked's Cam_HandleHotspots is the direct
// descendant: the same values plus a 0x1000 bit (0x1004/0x1024/0x10A4), and
// the hero water states are HERO_TYPE_SWIM/SURF (state type 0x11/0x12) and
// HERO_STATE_WADE (state 0x72 there, 0x73 here).
//
// Sphere-collision flag (camCollFlag), its pre-active copy
// (camCollFlagPrev), and bump/hotspot mode (camCollMode = hotspot | base):
#define CAM_COLL_FLAG_LAND 0x14      // default flag
#define CAM_COLL_FLAG_WATER 0x34     // hero in a water state
#define CAM_COLL_FLAG_ACTIVE 0x80    // OR'd in after saving the pre-active copy
#define CAM_COLL_MODE_BASE 0xB4
// Hotspot modes; the first set levelCamData.hotSpot* flag wins.
#define CAM_HOTSPOT_LAVA 0x100
#define CAM_HOTSPOT_DEATH_SAND 0xB00
#define CAM_HOTSPOT_QUICK_SAND 0x300
#define CAM_HOTSPOT_ICE_WATER 0xD00
#define CAM_HOTSPOT_WATER 0
// Hero water states mirrored into levelCamData: state type 0x11 or the next
// one (0x11/0x12 in Deadlocked's HERO_TYPE_ENUM, SWIM/SURF), or state 0x73.
#define CAM_HERO_STATE_TYPE_WATER 0x11
#define CAM_HERO_STATE_TYPE_WATER_SPAN 2
#define CAM_HERO_STATE_WADE 0x73
// camCollState's address is taken into a long-lived local (set first, used only
// in the mode chain below) so EGC hoists the base into the prologue and keeps
// the level-cam reads on the original registers; using the global directly
// materializes the base at point of use and breaks the byte match.
void Camera_updateCollMode(void) asm("func_001ED940");

void Camera_updateCollMode(void) {
    CamCollState* q;

    q = &camCollState;
    camCollFlag = CAM_COLL_FLAG_LAND;
    if ((u32)(levelCamData.inner.heroStateType - CAM_HERO_STATE_TYPE_WATER) <
        CAM_HERO_STATE_TYPE_WATER_SPAN ||
        levelCamData.inner.heroState == CAM_HERO_STATE_WADE) {
        camCollFlag = CAM_COLL_FLAG_WATER;
    }
    if (levelCamData.inner.heroStateType != CAM_HERO_STATE_TYPE_WATER &&
        levelCamData.inner.waterHeight < currentCamera.posZ) {
        camCollFlag = CAM_COLL_FLAG_LAND;
    }
    camCollFlagPrev = camCollFlag;
    camCollFlag |= CAM_COLL_FLAG_ACTIVE;
    camCollMode = CAM_COLL_MODE_BASE;
    if (levelCamData.inner.hotSpotLava != 0) {
        q->collMode = CAM_HOTSPOT_LAVA;
        camCollMode = CAM_HOTSPOT_LAVA | CAM_COLL_MODE_BASE;
    } else if (levelCamData.inner.hotSpotDeathSand != 0) {
        q->collMode = CAM_HOTSPOT_DEATH_SAND;
        camCollMode = CAM_HOTSPOT_DEATH_SAND | CAM_COLL_MODE_BASE;
    } else if (levelCamData.inner.hotSpotQuickSand != 0) {
        q->collMode = CAM_HOTSPOT_QUICK_SAND;
        camCollMode = CAM_HOTSPOT_QUICK_SAND | CAM_COLL_MODE_BASE;
    } else if (levelCamData.inner.hotSpotIceWater != 0) {
        q->collMode = CAM_HOTSPOT_ICE_WATER;
        camCollMode = CAM_HOTSPOT_ICE_WATER | CAM_COLL_MODE_BASE;
    } else if (levelCamData.inner.hotSpotWater != 0) {
        q->collMode = CAM_HOTSPOT_WATER;
        camCollMode = CAM_COLL_MODE_BASE;
    } else {
        camCollMode = q->collMode | CAM_COLL_MODE_BASE;
    }
}
// Per-frame screen-fade update (direct ancestor of Deadlocked's
// Camera_HandleScreenFade, called from the level-cam update between the
// camera timer bump and the hotspot check): the camera's own fade value is
// consumed from the global screen fade; when the global can no longer cover
// it, both are cleared. The unmangled asm label keeps the Splat symbol pin.
void Camera_HandleScreenFade(void) asm("func_001EDA60");

void Camera_HandleScreenFade(void) {
    float cur = currentCamera.screenFade;
    if (cur != 0.0f) {
        float next = screenFadeGp - cur;
        screenFadeGp = next;
        if (next <= 0.0f) {
            currentCamera.screenFade = 0.0f;
            screenFade = 0.0f;
        }
    }
}
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EDAA8);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EDC30);
