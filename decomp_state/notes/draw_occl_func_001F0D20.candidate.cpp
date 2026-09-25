#include "common.h"
#include "types.h"
#include "camera.h"
#include "pad_state.h"

// ---- callees ----
extern "C" void sound_update(void);
void SetBackgroundColor(s32 a, s32 b, s32 c); // -> SetBackgroundColor__Fiii
void UpdateFog(s32 a);                         // -> UpdateFog__Fi
void UpdateViewContext(void);                  // -> UpdateViewContext__Fv
extern "C" void FastVecSub(void* a, void* b, void* c);
extern "C" s32 func_001F9B20(void* a);         // linkage/ret unverified
extern "C" f32 func_001FA6C0(s32 a);           // linkage/ret unverified
f32 FastArcTan(f32 a, f32 b);                  // -> FastArcTan__Fff
f32 FastSubRots(f32 a, f32 b);                 // -> FastSubRots__Fff
extern "C" s32 sprintf(char* a, const char* b, ...);
extern "C" s32 strlen(const char* a);
extern "C" u64 fptodp(f32 a);
extern "C" s32 sceOpen(const char* a, s32 b);
extern "C" s32 sceClose(s32 a);
extern "C" s32 sceLseek(s32 a, s32 b, s32 c);
extern "C" s32 sceRead(s32 a, void* b, s32 c);
extern "C" s32 sceWrite(s32 a, void* b, s32 c);

// ---- data ----
extern u32 GameMode;
extern int currentLevelId;
extern PAD padState;
extern s32 occlColor0 __attribute__((section(".data")));
extern s32 occlColor1 __attribute__((section(".data")));
extern s32 occlColor2 __attribute__((section(".data")));
extern s32 occlFlag0 __attribute__((section(".data")));
extern s32 occlSampleCount __attribute__((section(".data")));
extern f32 occlSamplePoints[];
extern s32 occlValueTable[];
extern u8 occlFogCtr0;
extern u8 occlFogCtr1;
extern u8 occlFogCtr2;
extern f32 occlFogNear;
extern f32 occlFogFar;
extern f32 occlFogParam0;
extern f32 occlFogParam1;
extern f32 occlFogFarSet __attribute__((section(".data")));
extern s32 occlFogCount;
extern f32 occlFogNearSet __attribute__((section(".data")));
extern f32 occlFogDensitySet __attribute__((section(".data")));
extern s32 occlFogMode __attribute__((section(".data")));
extern f32 occlCamVec[];
extern f32 occlCamVecFloatA __attribute__((section(".data")));
extern f32 occlCamVecFloatB __attribute__((section(".data")));
extern s16 occlCamParam0 __attribute__((section(".data")));
extern s16 occlCamParam1 __attribute__((section(".data")));
extern u8 occlCharTable[];
extern u8 occlCharArray[];
extern s32 occlDefaultCount;
extern u8 occlSampleDefaults[24];
extern const char occlPathPrefix[];
extern const char occlSampleDeltasName[];
extern const char occlAddSampleFmt[];
extern const char occlStartPtFmt[];
extern const char occlPathFmt[];
extern const char occlSemiOne[];
extern const char occlSampleName[];
extern f32 occlCamScale0 __attribute__((section(".data")));
extern f32 occlCamScale1 __attribute__((section(".data")));
extern f32 occlCamScale2 __attribute__((section(".data")));
extern f32 occlCamScale3 __attribute__((section(".data")));
extern "C" u8 D_0013E520[];

// 0x1863D0: array of 0x4C structs, s16 field at +0x3E.
struct OcclU16A { u8 pad_3e[0x3E]; s16 value; };
extern OcclU16A occlU16TableABase[];
// 0x1DFFB0: array of 0x18 structs, s16 field at +0xE.
struct OcclU16B { u8 pad_e[0xE]; s16 value; };
extern OcclU16B occlU16TableBBase[];


// Dead tail (spimdis func_001F0CE0) prepended to the sampler block.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001F0CE0, 0x40\n"
    "glabel func_001F0CE0\n"
    "    .word 0x27bd02a0\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0010\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd02a0\n"
    "    .word 0x00000000\n"
    "    .word 0x00c0102d\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd00d0\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0010\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0150\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0060\n"
    "    .word 0x00000000\n"
    "endlabel func_001F0CE0\n"
    "    .set reorder\n"
    "    .set at\n"
);

void occlCamDebugSampler() asm("func_001F0D20");
void occlCamDebugSampler() {
    sound_update();

    s32 padButtons = padState.pressedButtons;
    if (padButtons & 0x500) {
        GameMode = occlCamState.bgColorCache;
        SetBackgroundColor(occlColor0, occlColor1, occlColor2);
        occlCamState.logFlag1 = 0x6e;
        occlCamState.logFlag0 = 0x6e;
        occlCamState.posIntZ = (s32)currentCamera.posZ >> 2;
        occlCamState.posIntX = (s32)currentCamera.pos >> 2;
        occlCamState.posIntY = (s32)currentCamera.posY >> 2;
        return;
    }

    char sampleBuf[24];
    memcpy(sampleBuf, (const char*)occlSampleDefaults, 24);
    int* buf = (int*)sampleBuf;

    if (padButtons & 0x2000) {
        occlCamState.state++;
        if (occlCamState.state == 3) {
            occlCamState.state = 0;
        } else if (occlCamState.state == 6) {
            occlCamState.state = 3;
        }
    }
    if (padButtons & 0x8000) {
        occlCamState.state--;
        if (occlCamState.state < 0) {
            occlCamState.state = 2;
        } else if (occlCamState.state == 2) {
            occlCamState.state = 5;
        }
    }
    if (padButtons & 0x1000) {
        occlCamState.subState--;
        if (occlCamState.subState < 0) {
            if (occlCamState.state >= 3) {
                occlCamState.state = (occlCamState.state + 3) / 6;
            }
            occlCamState.subState = buf[occlCamState.state] - 1;
            goto after0x4000;
        }
    }
    if (padButtons & 0x4000) {
        occlCamState.subState++;
        if (occlCamState.subState >= buf[occlCamState.state]) {
            if (occlCamState.state < 3) {
                occlCamState.state = (occlCamState.state + 9) % 6;
            }
            occlCamState.subState = 0;
        }
    }
after0x4000:
    if (buf[occlCamState.state] <= (int)occlCamState.subState) {
        occlCamState.subState = buf[occlCamState.state] - 1;
    }

    switch (occlCamState.state) {
    case 0: {
        if ((padButtons & 0x40) &&
            ((1 << (occlCamState.subState & 0x1f)) == 0x10)) {
            occlCamState.bits0 ^= (1 << (occlCamState.subState & 0x1f));
            if ((occlCamState.bits0 & 0x10) == 0) {
                occlCamState.bits0 = 0xf;
            } else {
                occlCamState.bits0 = 0x10;
            }
        }
        break;
    }
    case 1: {
        switch (occlCamState.subState) {
        case 0: {
            if (padButtons & 0x50) {
                if ((padButtons & 0x40) && (occlCamState.staged + 1 > 3)) {
                    occlCamState.staged++;
                    occlCamState.staged = 0;
                }
                if ((padButtons & 0x10) && (occlCamState.staged - 1 < 0)) {
                    occlCamState.staged--;
                    occlCamState.staged = 3;
                }
                if (occlCamState.staged == 1) {
                    occlCamState.bits0 = 0;
                } else if (occlCamState.staged < 2) {
                    if (occlCamState.staged == 0) {
                        occlCamState.bits0 = 0xf;
                    }
                } else if (occlCamState.staged == 2) {
                    occlCamState.bits0 = 0;
                    f32 work[4];
                    FastVecSub(work, occlCamVec, &currentCamera.pos);
                    func_001F9B20(work);
                    f32 at = FastArcTan(work[1], work[2]);
                    occlCamState.angleC = at;
                    occlCamState.angleB = FastSubRots(at, currentCamera.rot.radius);
                    f32 dz = occlCamVecFloatA - currentCamera.posZ;
                    occlCamState.angleD = dz;
                    occlCamState.angleA = FastSubRots(dz, currentCamera.rot.radius);
                } else if (occlCamState.staged == 3) {
                    occlCamState.bits0 = 6;
                }
            }
            break;
        }
        case 1: {
            if ((padButtons & 0x40) && (occlCamState.step1 + 1 > 7)) {
                occlCamState.step1++;
                occlCamState.step1 = 0;
            }
            if ((padButtons & 0x10) && (occlCamState.step1 - 1 < 0)) {
                occlCamState.step1--;
                occlCamState.step1 = 7;
            }
            break;
        }
        case 2: {
            if (occlFlag0 == 0) {
                occlCamState.step2 = 0;
            } else {
                if ((padButtons & 0x40) && (occlCamState.step2 + 1 > 2)) {
                    occlCamState.step2++;
                    occlCamState.step2 = 0;
                }
                if ((padButtons & 0x10) && (occlCamState.step2 - 1 < 0)) {
                    occlCamState.step2--;
                    occlCamState.step2 = 2;
                }
            }
            break;
        }
        case 3: {
            if (padButtons & 0x50) {
                occlCamState.flagA = (occlCamState.flagA == 0);
            }
            break;
        }
        case 4: {
            if (padButtons & 0x50) {
                occlCamState.flagB = (occlCamState.flagB == 0);
            }
            break;
        }
        case 5: {
            if (padButtons & 0x50) {
                occlCamState.flagC = (occlCamState.flagC == 0);
                if (occlCamState.flagC) {
                    occlFogFarSet = 1000.0f;
                    occlFogNearSet = 100.0f;
                    occlFogDensitySet = 60.0f;
                    occlFogCount = 0x40;
                    occlFogMode = 0x40000;
                } else {
                    occlFogFarSet = 5000.0f;
                    occlFogNearSet = 400.0f;
                    occlFogDensitySet = 200.0f;
                    occlFogCount = 0x1f4;
                    occlFogMode = 0x1f4000;
                }
            }
            break;
        }
        case 6: {
            if ((padButtons & 0x40) && (occlCamState.stepD + 1 > 2)) {
                occlCamState.stepD++;
                occlCamState.stepD = 0;
            }
            if ((padButtons & 0x10) && (occlCamState.stepD - 1 < 0)) {
                occlCamState.stepD--;
                occlCamState.stepD = 2;
            }
            break;
        }
        case 7: {
            if (padButtons & 0x50) {
                occlCamState.flagE = (occlCamState.flagE == 0);
            }
            break;
        }
        case 8: {
            if (padButtons & 0x50) {
                occlCamState.flagF = (occlCamState.flagF == 0);
                f32 scale = occlCamState.flagF ? 0.125f : 0.5f;
                occlCamScale0 = func_001FA6C0(occlCamParam0) * scale;
                occlCamScale1 = func_001FA6C0(occlCamParam1) * scale;
                occlCamScale2 = occlCamScale0 * 4.0f;
                occlCamScale3 = occlCamScale1 * 4.0f;
                UpdateViewContext();
            }
            break;
        }
        case 9: {
            if (padButtons & 0x50) {
                if ((padButtons & 0x40) == 0) {
                    occlCamState.stepG--;
                    if (occlCamState.stepG < -1) {
                        occlCamState.stepG = 0x23;
                    }
                } else {
                    occlCamState.stepG++;
                    if (occlCamState.stepG == 0x24) {
                        occlCamState.stepG = -1;
                    }
                }
            }
            if (occlCamState.stepG < 0) {
                occlCamState.bgColorCache = 0;
            } else {
                occlCamState.bgColorCache = 2;
            }
            break;
        }
        case 10: {
            if ((padButtons & 0x50) == 0) {
                if (padButtons & 0x20) {
                    s8 c = 0;
                    if (occlCharTable[occlCamState.index + currentLevelId * 0x10] == 0) {
                        c = -1;
                    }
                    occlCharTable[occlCamState.index + currentLevelId * 0x10] = c;
                    occlCharArray[occlCamState.index] = c;
                }
            } else if ((padButtons & 0x40) == 0) {
                occlCamState.index--;
                if (occlCamState.index < 0) {
                    occlCamState.index = 0xf;
                }
            } else {
                occlCamState.index++;
                if (occlCamState.index == 0x10) {
                    occlCamState.index = 0;
                }
            }
            break;
        }
        }
        break;
    }
    case 2: {
        if (padButtons & 0x40) {
            occlCamState.bits1 ^= (1 << (occlCamState.subState & 0x1f));
        }
        break;
    }
    case 3: {
        s32 sub = occlCamState.subState;
        int* target = &occlDefaultCount;
        s32 maxv = 999999;
        s32 v;
        if ((s32)sub < 0xd) {
            u8 idx = D_0013E520[sub];
            target = &occlValueTable[idx];
            maxv = occlU16TableBBase[idx].value;
        }
        u64 pad64 = *(u64*)&padState.field_1A0;
        if ((pad64 & 0x8) == 0) {
            if ((pad64 & 0x2) == 0) {
                if (padButtons & 0x40) {
                    *target = *target + 1;
                }
                v = *target;
                if (padButtons & 0x20) {
                    if (0 < v) {
                        *target = v - 1;
                    }
                }
            } else {
                if (padButtons & 0x40) {
                    *target = *target + 600;
                }
                if ((padButtons & 0x20) == 0) {
                    v = *target;
                } else {
                    if ((s32)*target < 0x259) {
                        *target = 0;
                    } else {
                        *target = *target - 600;
                    }
                    v = *target;
                }
            }
        } else {
            if (padButtons & 0x40) {
                *target = *target + 0x3c;
            }
            if (padButtons & 0x20) {
                if (0x3c < (s32)*target) {
                    *target = *target - 0x3c;
                } else {
                    *target = 0;
                }
                v = *target;
            }
        }
        if ((s32)maxv < (s32)v) {
            *target = maxv;
        }
        if (padButtons & 0x10) {
            u8 idx = D_0013E520[sub];
            D_0013E520[idx] = D_0013E520[idx] + 1;
        }
        if (padButtons & 0x80) {
            u8 idx = D_0013E520[sub];
            D_0013E520[idx] = D_0013E520[idx] - 1;
        }
        u8 idx = D_0013E520[sub];
        s16 tableVal = occlU16TableABase[idx].value;
        if (tableVal < (s16)D_0013E520[idx]) {
            D_0013E520[idx] = (u8)tableVal;
        }
        break;
    }
    case 4: {
        u64 pad64 = *(u64*)&padState.field_1A0;
        if (occlCamState.subState == 0) {
            if (pad64 & 0x800000040ULL) {
                occlFogNear = occlFogNear + 1024.0f;
            }
            if (pad64 & 0x400000010ULL) {
                occlFogNear = occlFogNear - 1024.0f;
            }
            if (occlFogNear <= occlFogFar) {
                if (occlFogNear < 0.0f) {
                    occlFogNear = 0.0f;
                }
            } else {
                occlFogFar = occlFogNear;
            }
        }
        if (occlCamState.subState == 2) {
            if (pad64 & 0x800000040ULL) {
                occlFogFar = occlFogFar + 1024.0f;
            }
            if (pad64 & 0x400000010ULL) {
                occlFogFar = occlFogFar - 1024.0f;
            }
            if (occlFogFar <= 524288.0f) {
                if (occlFogFar < occlFogNear) {
                    occlFogFar = occlFogNear;
                }
            } else {
                occlFogNear = 524288.0f;
            }
        }
        if (occlCamState.subState == 1) {
            if (pad64 & 0x800000040ULL) {
                occlFogParam0 = occlFogParam0 - 2.55f;
            }
            if (pad64 & 0x400000010ULL) {
                occlFogParam0 = occlFogParam0 + 2.55f;
            }
            if (occlFogParam0 <= 255.0f) {
                if (occlFogParam0 < 0.0f) {
                    occlFogParam0 = 0.0f;
                }
            } else {
                occlFogNear = 255.0f;
            }
        }
        if (occlCamState.subState == 3) {
            if (pad64 & 0x800000040ULL) {
                occlFogParam1 = occlFogParam1 - 2.55f;
            }
            if (pad64 & 0x400000010ULL) {
                occlFogParam1 = occlFogParam1 + 2.55f;
            }
            if (occlFogParam1 <= 255.0f) {
                if (occlFogParam1 < 0.0f) {
                    occlFogParam1 = 0.0f;
                }
            } else {
                occlFogNear = 255.0f;
            }
        }
        if (occlCamState.subState == 4) {
            if ((s8)occlFogCtr0 != -1 && (pad64 & 0x800000040ULL)) {
                occlFogCtr0 = occlFogCtr0 + 1;
            }
            if ((s8)occlFogCtr0 != 0 && (pad64 & 0x400000010ULL)) {
                occlFogCtr0 = occlFogCtr0 - 1;
            }
        }
        if (occlCamState.subState == 5) {
            if ((s8)occlFogCtr1 != -1 && (pad64 & 0x800000040ULL)) {
                occlFogCtr1 = occlFogCtr1 + 1;
            }
            if ((s8)occlFogCtr1 != 0 && (pad64 & 0x400000010ULL)) {
                occlFogCtr1 = occlFogCtr1 - 1;
            }
        }
        if (occlCamState.subState == 6) {
            if ((s8)occlFogCtr2 != -1 && (pad64 & 0x800000040ULL)) {
                occlFogCtr2 = occlFogCtr2 + 1;
            }
            if ((s8)occlFogCtr2 != 0 && (pad64 & 0x400000010ULL)) {
                occlFogCtr2 = occlFogCtr2 - 1;
            }
        }
        UpdateFog(0x10000000);
        break;
    }
    case 5: {
        if ((occlCamState.subState == 0) && (padButtons & 0x50)) {
            occlCamState.toggleH = (occlCamState.toggleH == 0);
        }
        if ((occlCamState.subState == 1) && (padButtons & 0x50)) {
            occlCamState.toggleI = (occlCamState.toggleI == 0);
        }
        if ((occlCamState.subState == 2) && (padButtons & 0xf0)) {
            s32 count = occlSampleCount;
            f32* pts = occlSamplePoints;
            int found = 0;
            if (count > 0) {
                int i = 0;
                do {
                    if (((pts[0] < currentCamera.pos + 0.5f) &&
                         (currentCamera.pos - 0.5f < pts[0])) &&
                        ((pts[1] < currentCamera.posY + 0.5f) &&
                         (currentCamera.posY - 0.5f < pts[1])) &&
                        ((pts[2] < currentCamera.posZ + 0.5f) &&
                         (currentCamera.posZ - 0.5f < pts[2]))) {
                        found = 1;
                        break;
                    }
                    i++;
                    pts += 3;
                } while (i < count);
            }
            if (found) {
                return;
            }
            char path[0x40];
            char line[0x40];
            sprintf(path, occlPathFmt, occlPathPrefix, currentLevelId,
                    occlSampleDeltasName, occlSemiOne);
            sprintf(line, occlAddSampleFmt, fptodp(currentCamera.pos),
                    fptodp(currentCamera.posY), fptodp(currentCamera.posZ));
            s32 h = sceOpen(path, 0x303);
            if (h > 0) {
                sceLseek(h, 0, 2);
                s32 len = strlen(line);
                sceWrite(h, line, len);
                sceClose(h);
                occlCamState.logFlag1 = 0x79;
            }
            sprintf(path, occlPathFmt, occlPathPrefix, currentLevelId,
                    occlSampleName, occlSemiOne);
            h = sceOpen(path, 0x303);
            if (h > 0) {
                sceLseek(h, 0, 2);
                sceWrite(h, &currentCamera.pos, 4);
                sceWrite(h, &currentCamera.posY, 4);
                sceWrite(h, &currentCamera.posZ, 4);
                sceLseek(h, 0, 0);
                s32 n = sceLseek(h, 0, 2);
                if (n < 0x100001) {
                    sceLseek(h, 0, 0);
                    sceRead(h, occlSamplePoints, n);
                    occlSampleCount = n / 0xc;
                } else {
                    occlSampleCount = 0;
                }
                sceClose(h);
            }
        }
        if ((occlCamState.subState == 3) && (padButtons & 0xf0)) {
            char path[0x40];
            char line[0x40];
            sprintf(path, occlPathFmt, occlPathPrefix, currentLevelId,
                    occlSampleDeltasName, occlSemiOne);
            sprintf(line, occlStartPtFmt, fptodp(currentCamera.pos),
                    fptodp(currentCamera.posY), fptodp(currentCamera.posZ));
            s32 h = sceOpen(path, 0x303);
            if (h > 0) {
                sceLseek(h, 0, 2);
                s32 len = strlen(line);
                sceWrite(h, line, len);
                sceClose(h);
                occlCamState.logFlag0 = 0x79;
            }
        }
        break;
    }
    }
}

// Trailing dead tail (spimdis func_001F2068) after the sampler epilogue.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .word 0x00000000\n"
    "    nonmatching func_001F2068, 0x4\n"
    "glabel func_001F2068\n"
    "    .word 0x27bd00b0\n"
    "endlabel func_001F2068\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

