#include "common.h"
#include "types.h"

typedef void (*SndCompleteProc)(int, u64);
typedef struct {
    SndCompleteProc done;
    u64 u_data;
} SndCommandReturnDef;

// 989 IOP sound-server RPC client state. sceSifBindRpc zeroes `bound`; the
// IOP's SIF tag 0x8000000A response handler (0x11B138) writes it non-zero
// once the RPC registration handshake completes, which the init loops poll.
typedef struct {
    int serve;
    int serve_id;
    int semaphore;
    int pad_0C;
    int status;
    int dma_data;
    int pad_18;
    int callback;
    int callback_arg;
    int bound;
} SndRpcServer;

typedef struct {
    int cd_busy;
    int pad_04[3];
    int cd_error;
    int pad_14[11];
} volatile SndCdStreamInfo;

extern int sceSifInitRpc(int enable);
extern int sceSifBindRpc(SndRpcServer* client, int rpc_num, int flags);
extern void func_00116078(void*, ...);
extern void snd_SendIOPCommandAndWait(int cmd, int count, void* data);
extern void snd_SendIOPCommandNoWait(int cmd, int count, void* data, int x, int y);

extern int* snd_batchCommandBuffers[2];
extern int snd_batchFreeBytes[2];
extern int* snd_batchReturnBuffers[2];
extern SndCommandReturnDef* snd_streamBuffers[2];
extern int snd_cdCallbackPending;
extern unsigned int snd_cdCallbackArg;
extern SndCompleteProc snd_cdCallbackFn;
extern u64 snd_cdCallbackData;
extern SndRpcServer snd_rpcServer __attribute__((section(".data")));
extern SndRpcServer snd_cdRpcServer __attribute__((section(".data")));
extern SndCdStreamInfo snd_cdStreamInfo;
extern int snd_commandBuffer1[0x400];
extern int snd_commandBuffer2[0x400];
extern int snd_returnBuffer1[0x110];
extern int snd_returnBuffer2[0x110];
extern SndCommandReturnDef snd_streamBuffer1[0x100];
extern SndCommandReturnDef snd_streamBuffer2[0x100];
extern int snd_SifBindRpcErrorString __attribute__((section(".data")));
extern int snd_989SndSourceFile __attribute__((section(".data")));

// Binds the 989 IOP sound-server RPCs (0x123456 command, 0x123457 CD),
// spinning until the IOP marks each client bound, then resets the CD
// callback state and sends the initial 4-byte CD stream status command.
//
// BLOCKED: the two spin-loop bodies match EGC byte-for-byte with a
// barrier-after-bind + fixed-register form (see
// decomp_state/notes/989snd_snd_StartSoundSystem.md), but 28 straight-line
// scheduling diffs remain in the prologue / loop-2 setup / final block.
INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_pre", snd_StartSoundSystem);
