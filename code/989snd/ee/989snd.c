#include "common.h"

extern void snd_SendIOPCommandAndWait(int cmd, int count, void* data);
extern void snd_SendIOPCommandNoWait(int cmd, int count, void* data, int x, int y);
extern void snd_FlushSoundCommands(void);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StartSoundSystem);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_FlushSoundCommands);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012DE60);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_GotReturns);

extern void* D_0015EC80;
extern int D_0015EC84;
extern int D_0015ECC4;

void snd_PrepareReturnBuffer(int* buf, int index) {
    D_0015EC84 = index;
    D_0015EC80 = buf;
    buf[index + 1] = 0;
    *buf = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012DF18);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_BankLoadByLoc);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E078);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_BankLoadFromEE_CB);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E198);

void snd_ResolveBankXREFS(void) {
    snd_SendIOPCommandNoWait(0x8, 0, 0, 0, 0);
}

void snd_UnloadBank(int a) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x6, 0x4, buf, 0, 0);
}

void snd_SetMasterVolume(int a, int b) {
    int buf[2];
    buf[0] = a;
    buf[1] = b;
    snd_SendIOPCommandNoWait(0x9, 0x8, buf, 0, 0);
}

void snd_SetPlaybackMode(int a) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0xB, 0x4, buf, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E270);

void snd_SetMixerMode(int a, int b) {
    int buf[2];
    buf[0] = a;
    buf[1] = b;
    snd_SendIOPCommandNoWait(0xD, 0x8, buf, 0, 0);
}

void snd_SetGroupVoiceRange(int a, int b, int c) {
    int buf[3];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    snd_SendIOPCommandNoWait(0x4E, 0xC, buf, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E2F8);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_PlaySoundVolPanPMPB);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E350);

void snd_StopSound(int id) {
    int data = id;
    snd_SendIOPCommandNoWait(0x15, 4, &data, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E398);

void snd_StopAllSounds(void) {
    snd_SendIOPCommandNoWait(0x18, 0, 0, 0, 0);
}

void snd_PauseAllSoundsInGroup(int a) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x16, 0x4, buf, 0, 0);
}

void snd_ContinueAllSoundsInGroup(int a) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x17, 0x4, buf, 0, 0);
}

void snd_SoundIsStillPlaying_CB(int a, int b, int c) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x19, 0x4, buf, b, c);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E478);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SetSoundParams_CB);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E508);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SendIOPCommandAndWait);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SendIOPCommandNoWait);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_PostMessage);

extern void FlushCache(int);
extern int D_00153D20 __attribute__((section(".data")));
extern int D_0015EBC0 __attribute__((section(".data")));
extern int* snd_batchCommandBuffers[2];
extern int snd_batchFreeBytes[2];
extern int* snd_batchReturnBuffers[2];
extern int snd_batchIndex;
extern int sceSifCheckStatRpc(void*);
extern void func_00116078(void*);
extern int sceSifCallRpc(void*, int, int, void*, int, void*, int, void (*)(void*), void*);

void snd_SendCurrentBatch(void) {
    int next;
    snd_PrepareReturnBuffer(snd_batchReturnBuffers[snd_batchIndex],
                           *snd_batchCommandBuffers[snd_batchIndex]);
    while (sceSifCheckStatRpc(&D_0015EBC0)) {
        func_00116078(&D_00153D20);
        FlushCache(0);
    }
    sceSifCallRpc(&D_0015EBC0, 0x4D, 1, snd_batchCommandBuffers[snd_batchIndex],
                  0x1000 - snd_batchFreeBytes[snd_batchIndex],
                  snd_batchReturnBuffers[snd_batchIndex],
                  *snd_batchCommandBuffers[snd_batchIndex] * 4 + 8, 0, 0);
    next = snd_batchIndex != 1;
    snd_batchIndex = next;
    *snd_batchCommandBuffers[next] = 0;
    snd_batchFreeBytes[next] = 0xFFC;
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_UnkFunction_0012eaf0);

void snd_UnkFunction_0012eb00(void) {
    D_0015ECC4 = 0;
    snd_FlushSoundCommands();
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_InitVAGStreamingEx);

void snd_StopAllStreams(void) {
    snd_SendIOPCommandNoWait(0x34, 0, 0, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012EC00);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_PlayVAGStreamByLocEx_CB);

void snd_PauseVAGStream(int a) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x2D, 0x4, buf, 0, 0);
}

void snd_ContinueVAGStream(int a) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x2E, 0x4, buf, 0, 0);
}

void snd_GetVAGStreamTimeRemaining_CB(int a, int b, int c) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x32, 0x4, buf, b, c);
}

void snd_IsVAGStreamBuffered_CB(int a, int b, int c) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x4F, 0x4, buf, b, c);
}

void snd_StreamSafeCheckCDIdle(int arg) {
    int buf[4];
    buf[0] = arg;
    snd_SendIOPCommandAndWait(0x36, 4, buf);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StreamSafeCdRead);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StreamSafeCdSync);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StreamSafeCdBreak);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StreamSafeCdGetError);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StreamSafeCdCallback);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012EF58);

void snd_SetReverbEx(int a, int b, int c, int d, int e) {
    int buf[5];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    buf[4] = e;
    snd_SendIOPCommandNoWait(0x50, 0x14, buf, 0, 0);
}

void snd_PreAllocReverbWorkArea(int a, int b) {
    int buf[2];
    buf[0] = a;
    buf[1] = b;
    snd_SendIOPCommandNoWait(0x51, 0x8, buf, 0, 0);
}

void snd_AutoReverb(int a, int b, int c, int d) {
    int buf[4];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    snd_SendIOPCommandNoWait(0x10, 0x10, buf, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012F020);

void snd_InitMovieSound(int a, int b, int c, int d, int e, int f) {
    int buf[6];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    buf[4] = e;
    buf[5] = f;
    snd_SendIOPCommandAndWait(0x3B, 0x18, buf);
}

void snd_ResetMovieSound(void) {
    snd_SendIOPCommandAndWait(0x3D, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012F0D0);

void snd_CloseMovieSound(void) {
    snd_SendIOPCommandAndWait(0x3C, 0, 0);
}

void snd_StartMovieSound(int a, int b, int c, int d, int e) {
    int buf[5];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    buf[4] = e;
    snd_SendIOPCommandAndWait(0x3E, 0x14, buf);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012F140);

void snd_UpdateMovieADPCM(int a, int b) {
    int buf[2];
    buf[0] = a;
    buf[1] = b;
    snd_SendIOPCommandAndWait(0x5A, 0x8, buf);
}

void snd_GetMovieNAX(void) {
    snd_SendIOPCommandAndWait(0x5B, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_GetDopplerPitchMod);
