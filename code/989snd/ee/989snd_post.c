#include "common.h"
#include "types.h"

typedef void (*SndCompleteProc)(int, u64);
typedef struct {
    SndCompleteProc done;
    u64 u_data;
} SndCommandReturnDef;
typedef struct {
    int num_commands;
    char buffer[4092];
} SndCommandBuffer;

extern unsigned int snd_SendIOPCommandAndWait(int cmd, int count, char* data);
extern void snd_SendIOPCommandNoWait(int command, int data_size, char* data, SndCompleteProc done, u64 u_data);
extern int snd_FlushSoundCommands(void);
extern void FlushCache(int);
extern int snd_rpcServer __attribute__((section(".data")));
extern int sceSifCheckStatRpc(void*);
extern int sceSifCallRpc(void*, int, int, void*, int, void*, int, void (*)(void*), void*);
extern void func_00116078(void*, ...);
extern int snd_batchBusy;
extern int snd_GotReturns(void);
extern int* snd_currentBuffer;
extern int snd_cdStreamActive;
extern int snd_cdCallbackPending;
extern int snd_StreamSafeCdSync(int);
// Synchronous command return area (16 words); word 1 is the IOP return word.
extern unsigned int snd_syncBuffer[16] __attribute__((section(".data")));
// Staging area copied to the IOP for synchronous command payloads.
extern char snd_syncSendBuffer[0x200] __attribute__((section(".data")));
// "989snd.c: RPC still nonidle!\n" error string reported by snd_SendCurrentBatch.
extern int snd_NonIdleErrorString __attribute__((section(".data")));
// "snd_SendIOPCommandNoWait: BUFFER %d FULL(%d)! ...\n" stalled-buffer error.
extern int snd_NoWaitBufferFullString __attribute__((section(".data")));
// "snd_SendIOPCommandNoWait: continueing (%d).\n" spin-wait progress report.
extern int snd_NoWaitContinuingString __attribute__((section(".data")));
extern SndCommandBuffer* snd_batchCommandBuffers[2];
extern int snd_batchFreeBytes[2];
extern int* snd_batchReturnBuffers[2];
extern int snd_batchIndex;
void snd_PrepareReturnBuffer(int* buf, int index);
extern void snd_SendCurrentBatch(void);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", func_0012E198);

void snd_ResolveBankXREFS(void) {
    snd_SendIOPCommandNoWait(0x8, 0, 0, 0, 0);
}

void snd_UnloadBank(int a) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x6, 0x4, (char*)buf, 0, 0);
}

void snd_SetMasterVolume(int a, int b) {
    int buf[2];
    buf[0] = a;
    buf[1] = b;
    snd_SendIOPCommandNoWait(0x9, 0x8, (char*)buf, 0, 0);
}

void snd_SetPlaybackMode(int a) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0xB, 0x4, (char*)buf, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", func_0012E270);

void snd_SetMixerMode(int a, int b) {
    int buf[2];
    buf[0] = a;
    buf[1] = b;
    snd_SendIOPCommandNoWait(0xD, 0x8, (char*)buf, 0, 0);
}

void snd_SetGroupVoiceRange(int a, int b, int c) {
    int buf[3];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    snd_SendIOPCommandNoWait(0x4E, 0xC, (char*)buf, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", func_0012E2F8);

void snd_PlaySoundVolPanPMPB(int a, int b, int c, int d, int e, int f,
                             SndCompleteProc g, u64 h) {
    int buf[6];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    buf[4] = e;
    buf[5] = f;
    snd_SendIOPCommandNoWait(0x11, 0x18, (char*)buf, g, h);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", func_0012E350);

void snd_StopSound(int id) {
    int data = id;
    snd_SendIOPCommandNoWait(0x15, 4, (char*)&data, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", func_0012E398);

void snd_StopAllSounds(void) {
    snd_SendIOPCommandNoWait(0x18, 0, 0, 0, 0);
}

void snd_PauseAllSoundsInGroup(int a) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x16, 0x4, (char*)buf, 0, 0);
}

void snd_ContinueAllSoundsInGroup(int a) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x17, 0x4, (char*)buf, 0, 0);
}

void snd_SoundIsStillPlaying_CB(int a, SndCompleteProc b, u64 c) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x19, 0x4, (char*)buf, b, c);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", func_0012E478);

void snd_SetSoundParams_CB(int a, int b, int c, int d, int e, int f,
                           SndCompleteProc g, u64 h) {
    int buf[6];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    buf[4] = e;
    buf[5] = f;
    snd_SendIOPCommandNoWait(0x21, 0x18, (char*)buf, g, h);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", func_0012E508);

// IOP RPC result region for a synchronous command: the three return words
// that snd_GotReturns validates after the call.
#define SND_SYNC_RPC_RESULT_SIZE 0xC

unsigned int snd_SendIOPCommandAndWait(int cmd, int count, char* data) {
    int i;
    int r;
    unsigned int ret;

    for (i = 0; i < count; i++)
        snd_syncSendBuffer[i] = data[i];
    while (snd_currentBuffer != 0) {
        snd_FlushSoundCommands();
        FlushCache(0);
    }
    snd_PrepareReturnBuffer(snd_syncBuffer, 1);
    while (sceSifCheckStatRpc(&snd_rpcServer) != 0) {
        func_00116078(&snd_NonIdleErrorString);
        snd_FlushSoundCommands();
        FlushCache(0);
    }
    if (count != 0) {
        sceSifCallRpc(&snd_rpcServer, cmd, 1, snd_syncSendBuffer, count,
                      snd_syncBuffer, SND_SYNC_RPC_RESULT_SIZE, 0, 0);
    } else {
        sceSifCallRpc(&snd_rpcServer, cmd, 1, 0, 0,
                      snd_syncBuffer, SND_SYNC_RPC_RESULT_SIZE, 0, 0);
    }
    do {
        r = snd_GotReturns();
        // The library's build pads the spin-loop body with three nops; the EE
        // assembler adds one more for the jal delay slot.
        asm volatile("nop\n\tnop\n\tnop");
    } while (r == 0);
    ret = snd_syncBuffer[1];
    if (snd_batchCommandBuffers[snd_batchIndex]->num_commands != 0 &&
        snd_batchBusy == 0) {
        snd_SendCurrentBatch();
    }
    return ret;
}

// Sends a command to the IOP sound server. When no synchronous command or
// batch transfer is in flight and the command has no payload or completion
// callback, it is issued synchronously as a fire-and-forget RPC. Otherwise it
// is appended to the current command batch (spinning until the batch has
// room), with the completion callback table updated, and snd_PostMessage
// advances the batch command count and flushes.
//
// Blocked: the body is fully understood (see
// decomp_state/notes/989snd_snd_SendIOPCommandNoWait.md) but the local EGC
// (SN 2.73a) allocates the spin-loop constants (256, 1, string base) into
// caller-saved s-registers and spills the incoming command/data/done
// parameters to the stack, while the original keeps all four parameters in
// s-registers and reloads the constants between calls. No C structure or
// flag found makes the local build reproduce the original 9-s-register
// allocation.
INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", snd_SendIOPCommandNoWait);

void snd_PostMessage(void) {
    SndCommandBuffer* commandBuffer = snd_batchCommandBuffers[snd_batchIndex];
    commandBuffer->num_commands++;
    snd_FlushSoundCommands();
}

void snd_SendCurrentBatch(void) {
    int nextIndex;

    snd_PrepareReturnBuffer(snd_batchReturnBuffers[snd_batchIndex],
                             snd_batchCommandBuffers[snd_batchIndex]->num_commands);
    while (sceSifCheckStatRpc(&snd_rpcServer)) {
        func_00116078(&snd_NonIdleErrorString);
        FlushCache(0);
    }

    sceSifCallRpc(&snd_rpcServer, 0x4D, 1, snd_batchCommandBuffers[snd_batchIndex],
                  0x1000 - snd_batchFreeBytes[snd_batchIndex],
                  snd_batchReturnBuffers[snd_batchIndex],
                  snd_batchCommandBuffers[snd_batchIndex]->num_commands * 4 + 8, 0, 0);
    nextIndex = snd_batchIndex != 1;
    snd_batchIndex = nextIndex;
    snd_batchCommandBuffers[nextIndex]->num_commands = 0;
    snd_batchFreeBytes[nextIndex] = 0xFFC;
}

void snd_UnkFunction_0012eaf0(void) {
    snd_batchBusy = 1;
}

void snd_UnkFunction_0012eb00(void) {
    snd_batchBusy = 0;
    snd_FlushSoundCommands();
}

// Initializes the IOP VAG streaming session. Already-initialized sessions
// short-circuit to 0; otherwise pending callbacks are drained, the CD is
// synced, and the init command goes out with the channel count, buffer
// size, read mode, and EE stream-safe enable. The IOP response becomes the
// new snd_cdStreamActive state.
int snd_InitVAGStreamingEx(int num_channels, int buffer_size,
                           unsigned int read_mode, int enable_streamsafe) {
    int data[4];
    int ret;
    int pending;

    if (snd_cdStreamActive == 1)
        return 0;
    pending = snd_cdCallbackPending;
    while (pending != 0) {
        pending = snd_FlushSoundCommands();
        // The library's build pads the spin-loop body with four nops; the
        // first fills the jal delay slot under the GNU macro assembler.
        asm volatile("nop\n\tnop\n\tnop\n\tnop");
    }
    snd_StreamSafeCdSync(0);
    data[0] = num_channels;
    data[1] = buffer_size;
    data[2] = read_mode;
    data[3] = enable_streamsafe;
    ret = snd_SendIOPCommandAndWait(0x2A, 0x10, (char*)data);
    snd_cdStreamActive = ret;
    return ret;
}

void snd_StopAllStreams(void) {
    snd_SendIOPCommandNoWait(0x34, 0, 0, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", func_0012EC00);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", snd_PlayVAGStreamByLocEx_CB);

void snd_PauseVAGStream(int a) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x2D, 0x4, (char*)buf, 0, 0);
}

void snd_ContinueVAGStream(int a) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x2E, 0x4, (char*)buf, 0, 0);
}

void snd_GetVAGStreamTimeRemaining_CB(int a, SndCompleteProc b, u64 c) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x32, 0x4, (char*)buf, b, c);
}

void snd_IsVAGStreamBuffered_CB(int a, SndCompleteProc b, u64 c) {
    int buf[1];
    buf[0] = a;
    snd_SendIOPCommandNoWait(0x4F, 0x4, (char*)buf, b, c);
}

void snd_StreamSafeCheckCDIdle(int arg) {
    int buf[4];
    buf[0] = arg;
    snd_SendIOPCommandAndWait(0x36, 4, buf);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", snd_StreamSafeCdRead);

// `volatile` keeps EGC from folding the out-of-window 0x137B00 base load into
// the branch delay slot; without it the 14-word shape does not match.
typedef struct {
    int cd_busy;
    int pad_04[3];
    int cd_error;
    int pad_14[11];
} volatile SndCdStreamInfo;

extern int snd_cdStatusCallback;
extern int snd_cdStreamEndPending;
extern SndCdStreamInfo snd_cdStreamInfo;
/* The generated SCE library provides this raw CD error/status getter. Its
 * exact SDK API identity is unresolved, so retain the address-based name in
 * this C TU. */
extern int func_00121630(void);
/* The generated SCE library registers CD callbacks with the disc driver when
 * no stream-safe session is active. Its exact SDK API identity is unresolved,
 * so retain the address-based name in this C TU. */
extern int func_00120678(int);
/* The generated SCE library breaks the active CD streaming read through the
 * IOP. Its exact SDK API identity is unresolved, so retain the address-based
 * name in this C TU. */
extern int func_001216C8(void);
/* The generated SCE library checks the CD streaming command queue state when
 * no stream-safe session is active. Its exact SDK API identity is unresolved,
 * so retain the address-based name in this C TU. */
extern int func_00120C30(int);

int snd_StreamSafeCdSync(int arg0) {
    int stateZero;

    if (!snd_cdStreamActive)
        return func_00120C30(arg0);

    FlushCache(0);
    stateZero = (snd_cdStreamInfo.cd_busy == 0);
    snd_cdStreamEndPending = stateZero;
    if (stateZero == 1)
        return 0;
    if (arg0 == 1)
        return 1;
    if (stateZero)
        return 0;
    do {
        snd_FlushSoundCommands();
        FlushCache(0);
        stateZero = (snd_cdStreamInfo.cd_busy == 0);
        snd_cdStreamEndPending = stateZero;
    } while (stateZero == 0);
    return 0;
}

int snd_StreamSafeCdBreak(void) {
    if (!snd_cdStreamActive)
        return func_001216C8();
    snd_SendIOPCommandNoWait(0x37, 0, 0, 0, 0);
    return 1;
}

int snd_StreamSafeCdGetError(void) {
    if (!snd_cdStreamActive)
        return func_00121630();
    return snd_cdStreamInfo.cd_error;
}

int snd_StreamSafeCdCallback(int callback) {
    int old;

    if (!snd_cdStreamActive)
        return func_00120678(callback);

    old = snd_cdStatusCallback;
    snd_cdStatusCallback = callback;
    return old;
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", func_0012EF58);

void snd_SetReverbEx(int a, int b, int c, int d, int e) {
    int buf[5];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    buf[4] = e;
    snd_SendIOPCommandNoWait(0x50, 0x14, (char*)buf, 0, 0);
}

void snd_PreAllocReverbWorkArea(int a, int b) {
    int buf[2];
    buf[0] = a;
    buf[1] = b;
    snd_SendIOPCommandNoWait(0x51, 0x8, (char*)buf, 0, 0);
}

void snd_AutoReverb(int a, int b, int c, int d) {
    int buf[4];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    snd_SendIOPCommandNoWait(0x10, 0x10, (char*)buf, 0, 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", func_0012F020);

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

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", func_0012F0D0);

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

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", func_0012F140);

void snd_UpdateMovieADPCM(int a, int b) {
    int buf[2];
    buf[0] = a;
    buf[1] = b;
    snd_SendIOPCommandAndWait(0x5A, 0x8, buf);
}

void snd_GetMovieNAX(void) {
    snd_SendIOPCommandAndWait(0x5B, 0, 0);
}

int snd_GetDopplerPitchMod(int arg0) {
    return arg0 * 1524 / 741;
}
