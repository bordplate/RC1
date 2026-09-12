#include "common.h"
#include "types.h"

extern u8 backupCam[];
extern u8 backupCamData[];
extern u32 curCam __attribute__((section(".data")));

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
INCLUDE_ASM("code/_generated/nonmatchings/game/camera", func_001EC868);
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
