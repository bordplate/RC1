#include "types.h"

typedef unsigned int CameraQuad __attribute__((mode(TI)));

struct CameraControlActivation {
    float blendSpeed;   // 0x78
    u8 priority;        // 0x7C
    u8 activate;        // 0x7D
    s16 deactivate;     // 0x7E
    s16 repCam;         // 0x80
    s16 orgCam;         // 0x82
};

struct UpdateCam {
    CameraQuad mtx0;    // 0x00
    CameraQuad mtx1;    // 0x10
    CameraQuad mtx2;    // 0x20
    CameraQuad posQuad; // 0x30
    char pad_40[0x24];
    float lPos[3];      // 0x64
    u32 control;        // 0x70
    int activationType; // 0x74
    CameraControlActivation activation; // 0x78
    s16 importCameraIdx; // 0x84
    s16 collMode;        // 0x86
    s16 pad_88[2];
    s16 funcIdx;         // 0x8C
    s16 active;          // 0x8E
    char pad_90[0x10];
};

struct UpdateCamVtbl {
    int field_0x00;
    int (*activationCheck)(UpdateCam*, UpdateCam*);
    void (*runSetupToNewCam)(UpdateCam*);
    void (*collWithHero)(UpdateCam*);
    void (*exit)(UpdateCam*);
};
extern UpdateCamVtbl lvlCamVtbl[];

extern u32 curCam __attribute__((section(".data")));
extern UpdateCam updateCams[48] __attribute__((section(".data")));
extern int updateCamsUsed[48] __attribute__((section(".data")));

extern "C" int Camera_ActivationCheckPriority(UpdateCam* pCam, UpdateCam* pCurCam);
extern "C" void func_001EBF10(UpdateCam* pNewCam);
extern "C" void ExecuteCamPostUpdFuncs(void);
void Camera_Exit(UpdateCam* cam);
void Camera_handleCollWithHero(int camPtr, UpdateCam* pCam);

int UpdateAllCameras(int cameraIndex) {
    UpdateCam* cur = (UpdateCam*)curCam;
    Camera_Exit(cur);
    int changed = 0;
    int i = 0x2F;
    UpdateCam* pCam = updateCams;
    int* pUsed = updateCamsUsed;
    int r;
    do {
        if (*pUsed != 0 && pCam != cur &&
            (r = Camera_ActivationCheckPriority(pCam, cur)) != 0) {
            cur = pCam;
            changed = 1;
        }
        pCam++;
        i--;
        pUsed++;
    } while (i >= 0);
    if (changed) {
        func_001EBF10(cur);
    }
    s16 funcIdx = cur->funcIdx;
    void (*coll)(UpdateCam*) = lvlCamVtbl[funcIdx].collWithHero;
    Camera_handleCollWithHero((int)cur, (UpdateCam*)sizeof(UpdateCamVtbl));
    if (coll) {
        coll(cur);
    }
    float* pos = (float*)&cur->posQuad;
    float* dst = cur->lPos;
    dst[0] = pos[0];
    dst[1] = pos[1];
    dst[2] = pos[2];
    ExecuteCamPostUpdFuncs();
    return -1;
}
