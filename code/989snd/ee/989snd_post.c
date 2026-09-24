#include "common.h"
#include "types.h"
#include "989snd_iop.h"

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

// Unreachable dead tail the original compiler emitted after
// snd_BankLoadFromEE_CB (989snd_bankload.c): two 0x50 stack deallocations
// matching its 0x50 frame, plus the alignment nop before
// snd_ResolveBankXREFS. EGC 2.95.2 never regenerates a dead frame
// deallocation after the epilogue (probed; see
// decomp_state/notes/989snd_func_0012E198.md), so the bytes are preserved
// with raw asm.
asm("addiu $sp,$sp,0x50");
asm("nop");
asm("addiu $sp,$sp,0x50");
asm("nop");

void snd_ResolveBankXREFS(void) {
    snd_SendIOPCommandNoWait(SND_IOP_CMD_RESOLVE_BANK_XREFS, 0, 0, 0, 0);
}

void snd_UnloadBank(int bank) {
    int buf[1];
    buf[0] = bank;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_UNLOAD_BANK, 0x4, (char*)buf, 0, 0);
}

void snd_SetMasterVolume(int which, int vol) {
    int buf[2];
    buf[0] = which;
    buf[1] = vol;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_SET_MASTER_VOLUME, 0x8, (char*)buf, 0, 0);
}

void snd_SetPlaybackMode(int mode) {
    int buf[1];
    buf[0] = mode;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_SET_PLAYBACK_MODE, 0x4, (char*)buf, 0, 0);
}

// Unreachable dead tail the original compiler emitted after
// snd_SetPlaybackMode: two 0x10 stack deallocations plus the
// alignment nop before snd_SetMixerMode. EGC 2.95.2 never
// regenerates a dead frame deallocation after the epilogue
// (probed; see decomp_state/notes/989snd_func_0012E270.md), so
// the bytes are preserved with raw asm.
asm(".align 3");
asm("addiu $sp,$sp,0x10");
asm("nop");
asm("addiu $sp,$sp,0x10");
asm("nop");

void snd_SetMixerMode(int channel_mode, int reverb_mode) {
    int buf[2];
    buf[0] = channel_mode;
    buf[1] = reverb_mode;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_SET_MIXER_MODE, 0x8, (char*)buf, 0, 0);
}

void snd_SetGroupVoiceRange(int group, int min, int max) {
    int buf[3];
    buf[0] = group;
    buf[1] = min;
    buf[2] = max;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_SET_GROUP_VOICE_RANGE, 0xC, (char*)buf, 0, 0);
}

// Unreachable dead tail the original compiler emitted after
// snd_SetGroupVoiceRange: two 0x30 stack deallocations plus the
// alignment nop before snd_PlaySoundVolPanPMPB. EGC 2.95.2 never
// regenerates a dead frame deallocation after the epilogue
// (probed; see decomp_state/notes/989snd_func_0012E2F8.md), so
// the bytes are preserved with raw asm.
asm(".align 3");
asm("addiu $sp,$sp,0x30");
asm("nop");
asm("addiu $sp,$sp,0x30");
asm("nop");

void snd_PlaySoundVolPanPMPB(int bank, int sound, int vol, int pan,
                             int pitch_mod, int bend, SndCompleteProc cb,
                             u64 user_data) {
    int buf[6];
    buf[0] = bank;
    buf[1] = sound;
    buf[2] = vol;
    buf[3] = pan;
    buf[4] = pitch_mod;
    buf[5] = bend;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_PLAY_SOUND, 0x18, (char*)buf, cb, user_data);
}

// Unreachable dead tail the original compiler emitted after
// snd_PlaySoundVolPanPMPB: three 0x10 stack deallocations plus
// the alignment nop before snd_StopSound. EGC 2.95.2 never
// regenerates a dead frame deallocation after the epilogue
// (probed; see decomp_state/notes/989snd_func_0012E350.md), so
// the bytes are preserved with raw asm.
asm(".align 3");
asm("addiu $sp,$sp,0x10");
asm("nop");
asm("addiu $sp,$sp,0x10");
asm("nop");
asm("addiu $sp,$sp,0x10");
asm("nop");

void snd_StopSound(int handle) {
    int data = handle;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_STOP_SOUND, 4, (char*)&data, 0, 0);
}

// Unreachable dead tail the original compiler emitted after
// snd_StopSound: 0x40, 0x40, 0x20, 0x30 stack deallocations
// plus the alignment nop before snd_StopAllSounds. EGC 2.95.2
// never regenerates a dead frame deallocation after the
// epilogue (probed; see decomp_state/notes/989snd_func_0012E398.md),
// so the bytes are preserved with raw asm.
asm(".align 3");
asm("addiu $sp,$sp,0x40");
asm("nop");
asm("addiu $sp,$sp,0x40");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x30");
asm("nop");

void snd_StopAllSounds(void) {
    snd_SendIOPCommandNoWait(SND_IOP_CMD_STOP_ALL_SOUNDS, 0, 0, 0, 0);
}

void snd_PauseAllSoundsInGroup(int groups) {
    int buf[1];
    buf[0] = groups;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_PAUSE_ALL_SOUNDS_IN_GROUP, 0x4, (char*)buf, 0, 0);
}

void snd_ContinueAllSoundsInGroup(int groups) {
    int buf[1];
    buf[0] = groups;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_CONTINUE_ALL_SOUNDS_IN_GROUP, 0x4, (char*)buf, 0, 0);
}

void snd_SoundIsStillPlaying_CB(int handle, SndCompleteProc cb, u64 user_data) {
    int buf[1];
    buf[0] = handle;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_SOUND_IS_STILL_PLAYING, 0x4, (char*)buf, cb, user_data);
}

// Unreachable dead tail the original compiler emitted after
// snd_SoundIsStillPlaying_CB: eight 0x20 stack deallocations
// and one 0x30, plus the alignment nop before
// snd_SetSoundParams_CB. EGC 2.95.2 never regenerates a dead
// frame deallocation after the epilogue (probed; see
// decomp_state/notes/989snd_func_0012E478.md), so the bytes
// are preserved with raw asm.
asm(".align 3");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x30");
asm("nop");

void snd_SetSoundParams_CB(int handle, int mask, int vol, int pan,
                           int pitch_mod, int bend, SndCompleteProc cb,
                           u64 user_data) {
    int buf[6];
    buf[0] = handle;
    buf[1] = mask;
    buf[2] = vol;
    buf[3] = pan;
    buf[4] = pitch_mod;
    buf[5] = bend;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_SET_SOUND_PARAMS, 0x18, (char*)buf, cb, user_data);
}

// Unreachable dead tail the original compiler emitted after
// snd_SetSoundParams_CB: six 0x20 stack deallocations and two
// 0x10, plus the alignment nop before snd_SendIOPCommandAndWait.
// EGC 2.95.2 never regenerates a dead frame deallocation after
// the epilogue (probed; see
// decomp_state/notes/989snd_func_0012E508.md), so the bytes
// are preserved with raw asm.
asm(".align 3");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x10");
asm("nop");
asm("addiu $sp,$sp,0x10");
asm("nop");

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

    sceSifCallRpc(&snd_rpcServer, SND_IOP_CMD_EXECUTE_BATCH, 1,
                  snd_batchCommandBuffers[snd_batchIndex],
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
    snd_StreamSafeCdSync(SND_CD_SYNC_MODE_WAIT);
    data[0] = num_channels;
    data[1] = buffer_size;
    data[2] = read_mode;
    data[3] = enable_streamsafe;
    ret = snd_SendIOPCommandAndWait(SND_IOP_CMD_INIT_VAG_STREAMING, 0x10, (char*)data);
    snd_cdStreamActive = ret;
    return ret;
}

void snd_StopAllStreams(void) {
    snd_SendIOPCommandNoWait(SND_IOP_CMD_STOP_ALL_STREAMS, 0, 0, 0, 0);
}

// Unreachable dead tail the original compiler emitted after
// snd_StopAllStreams: one 0x10 stack deallocation plus the
// alignment nop before snd_PlayVAGStreamByLocEx_CB. EGC 2.95.2
// never regenerates a dead frame deallocation after the epilogue
// (probed; see decomp_state/notes/989snd_snd_StopAllStreams.md),
// so the bytes are preserved with raw asm.
asm(".align 3");
asm("addiu $sp,$sp,0x10");
asm("nop");

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_post", snd_PlayVAGStreamByLocEx_CB);

void snd_PauseVAGStream(int stream) {
    int buf[1];
    buf[0] = stream;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_PAUSE_VAG_STREAM, 0x4, (char*)buf, 0, 0);
}

void snd_ContinueVAGStream(int stream) {
    int buf[1];
    buf[0] = stream;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_CONTINUE_VAG_STREAM, 0x4, (char*)buf, 0, 0);
}

void snd_GetVAGStreamTimeRemaining_CB(int stream, SndCompleteProc cb,
                                      u64 user_data) {
    int buf[1];
    buf[0] = stream;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_GET_VAG_STREAM_TIME_REMAINING, 0x4, (char*)buf, cb, user_data);
}

void snd_IsVAGStreamBuffered_CB(int stream, SndCompleteProc cb, u64 user_data) {
    int buf[1];
    buf[0] = stream;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_IS_VAG_STREAM_BUFFERED, 0x4, (char*)buf, cb, user_data);
}

void snd_StreamSafeCheckCDIdle(int block_ee_iop) {
    int buf[4];
    buf[0] = block_ee_iop;
    snd_SendIOPCommandAndWait(SND_IOP_CMD_CHECK_CD_IDLE, 4, buf);
}

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
extern int snd_cdSyncPending;
extern SndCdStreamInfo snd_cdStreamInfo;
/* The generated SCE library performs the raw CD streaming read when no
 * stream-safe session is active. Its exact SDK API identity is unresolved, so
 * retain the address-based name in this C TU. */
extern int func_00121450(int, int, int);
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

// Issues a raw CD streaming read while a stream-safe session is active:
// sync the CD, mark the stream busy and clear the error, queue the read
// command, then flag the sync/stream-end pending states.
int snd_StreamSafeCdRead(int lbn, int sectors, int buf) {
    int data[3];

    if (!snd_cdStreamActive)
        return func_00121450(lbn, sectors, buf);
    if (snd_StreamSafeCdSync(SND_CD_SYNC_MODE_CHECK) == 1)
        return 0;
    snd_cdStreamInfo.cd_busy = 1;
    snd_cdStreamInfo.cd_error = 0;
    data[0] = lbn;
    data[1] = sectors;
    data[2] = buf;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_CD_STREAM_READ, 0xC, (char*)data, 0, 0);
    snd_cdSyncPending = 1;
    snd_cdStreamEndPending = 0;
    return 1;
}

int snd_StreamSafeCdSync(int mode) {
    int stateZero;

    if (!snd_cdStreamActive)
        return func_00120C30(mode);

    FlushCache(0);
    stateZero = (snd_cdStreamInfo.cd_busy == 0);
    snd_cdStreamEndPending = stateZero;
    if (stateZero == 1)
        return 0;
    if (mode == SND_CD_SYNC_MODE_CHECK)
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
    snd_SendIOPCommandNoWait(SND_IOP_CMD_BREAK_CD_STREAM_READ, 0, 0, 0, 0);
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

// Unreachable dead tail the original compiler emitted after
// snd_StreamSafeCdCallback: two 0x20 stack deallocations plus
// the alignment nop before snd_SetReverbEx. EGC 2.95.2 never
// regenerates a dead frame deallocation after the epilogue
// (probed; see decomp_state/notes/989snd_func_0012EF58.md), so
// the bytes are preserved with raw asm.
asm(".align 3");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");

void snd_SetReverbEx(int core, int type, int depth, int delay, int feedback) {
    int buf[5];
    buf[0] = core;
    buf[1] = type;
    buf[2] = depth;
    buf[3] = delay;
    buf[4] = feedback;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_SET_REVERB, 0x14, (char*)buf, 0, 0);
}

void snd_PreAllocReverbWorkArea(int core, int type) {
    int buf[2];
    buf[0] = core;
    buf[1] = type;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_PREALLOC_REVERB_WORK_AREA, 0x8, (char*)buf, 0, 0);
}

void snd_AutoReverb(int core, int depth, int delta_time, int channel_flags) {
    int buf[4];
    buf[0] = core;
    buf[1] = depth;
    buf[2] = delta_time;
    buf[3] = channel_flags;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_AUTO_REVERB, 0x10, (char*)buf, 0, 0);
}

// Unreachable dead tail the original compiler emitted after
// snd_AutoReverb: 0x10, 0x20, 0x10, 0x20, 0x20, 0x10, 0x10,
// 0x10, 0x10 stack deallocations plus the alignment nop before
// snd_InitMovieSound. EGC 2.95.2 never regenerates a dead frame
// deallocation after the epilogue (probed; see
// decomp_state/notes/989snd_func_0012F020.md), so the bytes
// are preserved with raw asm.
asm(".align 3");
asm("addiu $sp,$sp,0x10");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x10");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");
asm("addiu $sp,$sp,0x10");
asm("nop");
asm("addiu $sp,$sp,0x10");
asm("nop");
asm("addiu $sp,$sp,0x10");
asm("nop");
asm("addiu $sp,$sp,0x10");
asm("nop");

void snd_InitMovieSound(int sizeOfIOPBuffer, int sizeOfSPUBuffer, int volumeLevel,
                        int panCenter, int volumeGroup, int type) {
    int buf[6];
    buf[0] = sizeOfIOPBuffer;
    buf[1] = sizeOfSPUBuffer;
    buf[2] = volumeLevel;
    buf[3] = panCenter;
    buf[4] = volumeGroup;
    buf[5] = type;
    snd_SendIOPCommandAndWait(SND_IOP_CMD_INIT_MOVIE_SOUND, 0x18, buf);
}

void snd_ResetMovieSound(void) {
    snd_SendIOPCommandAndWait(SND_IOP_CMD_RESET_MOVIE_SOUND, 0, 0);
}

// Unreachable dead tail the original compiler emitted after
// snd_ResetMovieSound: a 0x10 and a 0x20 stack deallocation
// plus the alignment nop before snd_CloseMovieSound. EGC 2.95.2
// never regenerates a dead frame deallocation after the epilogue
// (probed; see decomp_state/notes/989snd_func_0012F0D0.md), so
// the bytes are preserved with raw asm.
asm(".align 3");
asm("addiu $sp,$sp,0x10");
asm("nop");
asm("addiu $sp,$sp,0x20");
asm("nop");

void snd_CloseMovieSound(void) {
    snd_SendIOPCommandAndWait(SND_IOP_CMD_CLOSE_MOVIE_SOUND, 0, 0);
}

void snd_StartMovieSound(int iopBuffer, int iopBufferSize, int iopPausePosition,
                         int sr, int ch) {
    int buf[5];
    buf[0] = iopBuffer;
    buf[1] = iopBufferSize;
    buf[2] = iopPausePosition;
    buf[3] = sr;
    buf[4] = ch;
    snd_SendIOPCommandAndWait(SND_IOP_CMD_START_MOVIE_SOUND, 0x14, buf);
}

// Unreachable dead tail the original compiler emitted after
// snd_StartMovieSound: one 0x10 stack deallocation plus the
// alignment nop before snd_UpdateMovieADPCM. EGC 2.95.2 never
// regenerates a dead frame deallocation after the epilogue
// (probed; see decomp_state/notes/989snd_func_0012F140.md), so
// the bytes are preserved with raw asm.
asm(".align 3");
asm("addiu $sp,$sp,0x10");
asm("nop");

void snd_UpdateMovieADPCM(int data_size, int offset) {
    int buf[2];
    buf[0] = data_size;
    buf[1] = offset;
    snd_SendIOPCommandAndWait(SND_IOP_CMD_UPDATE_MOVIE_ADPCM, 0x8, buf);
}

void snd_GetMovieNAX(void) {
    snd_SendIOPCommandAndWait(SND_IOP_CMD_GET_MOVIE_NAX, 0, 0);
}

int snd_GetDopplerPitchMod(int approaching_mph) {
    return approaching_mph * 1524 / 741;
}
