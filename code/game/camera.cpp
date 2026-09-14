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

INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EBD60);

INCLUDE_ASM("code/_generated/nonmatchings/game/camera", Cam_InterpValues__FffPffff);

INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EBE60);

INCLUDE_ASM("code/_generated/nonmatchings/game/camera", Camera_handleCollWithHero__FiP9UpdateCam);

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
