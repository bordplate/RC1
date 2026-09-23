#include "common.h"
#include "types.h"

extern u8 backupCam[];
extern u8 backupCamData[];
extern u32 curCam __attribute__((section(".data")));

// A 128-bit (16-byte) quadword. On the R5900 EGC lowers a `mode(TI)` value to
// lq/sq (128-bit) transfers; a plain 64-bit `long` lowers to ld/sd instead.
typedef unsigned int CameraQuad __attribute__((mode(TI)));

// Camera transition state block (0xE0 bytes at 0x1871B0). Holds the active
// camera transform (+0x50/+0x60) and the pending transform (+0xC0/+0xD0) the
// level camera code stages before a mode switch; each is a 16-byte quadword.
struct CameraTransState {
    u16 state;        // 0x00: transition status (caller compares against 1 and 2)
    u8 mode;          // 0x02: nonzero while a staged transform is pending commit
    u8 pendingMode;   // 0x03: staged camera mode (read by the caller)
    u8 pad_04[0x4C];
    CameraQuad activeCam0;  // 0x50
    CameraQuad activeCam1;  // 0x60
    u8 pad_70[0x50];
    CameraQuad pendingCam0; // 0xC0
    CameraQuad pendingCam1; // 0xD0
};
extern CameraTransState camTransState __attribute__((section(".data")));

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
// `type` at this spot, 6 bytes before UpdateCam::camType.
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

INCLUDE_ASM("code/_generated/nonmatchings/game/camera", Camera_runSetupToNewCam__FP9UpdateCam);

INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EBF10);

INCLUDE_ASM("code/_generated/nonmatchings/game/camera", Camera_ActivationCheckPriority);

// 0xA0-byte camera state block (see UpdateAllCameras__Fi iteration and
// BackupCurrentCam); per-level behavior is selected through lvlCamVtbl.
struct UpdateCam {
    char pad_00[0x8C];
    short camType;
    char pad_8E[0x12];
};

// One lvl.camvtbl entry (0x14 bytes), indexed by UpdateCam::camType; each
// level overlay supplies its own table at the same address.
struct UpdateCamVtbl {
    int field_0x00;
    int (*activationCheck)(UpdateCam*, UpdateCam*);
    void (*runSetupToNewCam)(UpdateCam*);
    void (*collWithHero)(UpdateCam*, float);
    void (*exit)(UpdateCam*);
};
extern UpdateCamVtbl lvlCamVtbl[];

void Camera_Exit(UpdateCam* cam) {
    void (*fn)(UpdateCam*) = lvlCamVtbl[cam->camType].exit;
    if (fn)
        fn(cam);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/camera", UpdateAllCameras__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EC530);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EC710);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EC7F0);
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
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ECAF8);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ECCD8);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ED2B0);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ED360);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ED470);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ED7F0);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001ED940);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EDA60);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EDAA8);
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EDC30);
