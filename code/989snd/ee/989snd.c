#include "common.h"

extern void snd_SendIOPCommandAndWait(int cmd, int count, void* data);
extern void snd_SendIOPCommandNoWait(int cmd, int count, void* data, int x, int y);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StartSoundSystem);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_FlushSoundCommands);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012DE60);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_GotReturns);

extern void* D_0015EC80;
extern int D_0015EC84;

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

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_ResolveBankXREFS);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_UnloadBank);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SetMasterVolume);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SetPlaybackMode);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E270);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SetMixerMode);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SetGroupVoiceRange);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E2F8);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_PlaySoundVolPanPMPB);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E350);

void snd_StopSound(int id) {
    int data = id;
    snd_SendIOPCommandNoWait(0x15, 4, &data, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E398);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StopAllSounds);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_PauseAllSoundsInGroup);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_ContinueAllSoundsInGroup);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SoundIsStillPlaying_CB);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E478);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SetSoundParams_CB);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012E508);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SendIOPCommandAndWait);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SendIOPCommandNoWait);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_PostMessage);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SendCurrentBatch);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_UnkFunction_0012eaf0);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_UnkFunction_0012eb00);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_InitVAGStreamingEx);

void snd_StopAllStreams(void) {
    snd_SendIOPCommandNoWait(0x34, 0, 0, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012EC00);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_PlayVAGStreamByLocEx_CB);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_PauseVAGStream);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_ContinueVAGStream);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_GetVAGStreamTimeRemaining_CB);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_IsVAGStreamBuffered_CB);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StreamSafeCheckCDIdle);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StreamSafeCdRead);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StreamSafeCdSync);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StreamSafeCdBreak);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StreamSafeCdGetError);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StreamSafeCdCallback);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012EF58);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_SetReverbEx);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_PreAllocReverbWorkArea);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_AutoReverb);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012F020);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_InitMovieSound);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_ResetMovieSound);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012F0D0);

void snd_CloseMovieSound(void) {
    snd_SendIOPCommandAndWait(0x3C, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_StartMovieSound);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", func_0012F140);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_UpdateMovieADPCM);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_GetMovieNAX);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd", snd_GetDopplerPitchMod);
