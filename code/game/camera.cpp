#include "common.h"
#include "types.h"
#include "camera.h"
#include "mobyutil.h"

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
// 0x18C32C, 8 bytes before OcclUpdate: nonzero while the occlusion subsystem
// stages its own camera transform, in which case camera switches and
// occlusion-visibility setup skip committing to currentCamera. Unconfirmed.
extern int occlCamStaged __attribute__((section(".data")));
// 0x15ED84: current level id; the polar/pos blend falls back to 0.01f on level 1.
extern int currentLevelId __attribute__((section(".data")));
// 0x1FA6D0 (fastfunc): truncates its float argument toward zero. C linkage.
extern "C" int func_001FA6D0(float x);

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

// Camera-collision state block at 0x1870D0; the current collision moby
// (GameCamera, 0x187194) sits at +0xC4. Moby pointers live below 0x10000000
// so the block stores/compares them as 32-bit values. The spawn reuses the
// single hoisted base register, so its position argument is a base offset.
struct CamCollState {
    char pad_c4[0xC4];
    MobyInstance* pCamColl; // +0xC4 -> 0x187194
};
extern CamCollState camCollState __attribute__((section(".data")));

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
// 64-byte matrix written by the transition orientation helpers.
struct CameraMatrix {
    CameraQuad q[4];
};
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
// Struct whose +0x14 u32 is the occlusion-staged flag (occlCamStaged,
// 0x18C32C); the original loads the base (0x18C318) into s3 and reads +0x14.
struct D18C318_t {
    u8 pad_14[0x14];
    u32 flag; // +0x14
};
extern f32 D_0015ED60;
extern struct D18C318_t D_0018C318;
extern CameraMatrix D_00187290;
// 16-byte quad at 0x187080 (== &currentCamera.pos); the original names this
// data symbol "Camera", which collides with struct Camera, so bind it via a
// symbol override.
extern CameraQuad camPos16 asm("Camera");

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

    if (D_0018C318.flag == 0) {
        register CameraQuad* dst asm("$3") = &camPos16;
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
    register f32 scale asm("$f0") = D_0015ED60;
    if (D_0018C318.flag == 0) {
        func_001FA2B8(&D_00187290, &work.mat);
        scale = D_0015ED60;
    }
    f32 nc = pB->field_1C + pB->posInterp * scale;
    pB->field_1C = nc;
    if (1.0f < nc)
        pB->field_1C = 1.0f;
    scale = D_0015ED60;
    f32 nn = pB->field_10 + pB->quatInterp * scale;
    pB->field_10 = nn;
    if (1.0f < nn)
        pB->field_10 = 1.0f;
    return 0;
}
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ECCD8);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ED2B0);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ED360);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ED470);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ED7F0);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ED940);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EDA60);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EDAA8);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EDC30);
