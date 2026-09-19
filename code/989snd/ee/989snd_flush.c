#include "common.h"
#include "types.h"

typedef void (*SndCompleteProc)(int, u64);
typedef void (*SndCdCallback)(int);

// SN single-pass assembler needs the small-data globals declared before first
// use so their bare pseudos expand GPREL; the CD callback arg/data are left
// unseeded on purpose (they expand self-based in the macro regions the
// original uses).
asm(".extern snd_currentBuffer, 4\n"
    ".extern snd_returnCallbackPending, 4\n"
    ".extern snd_returnCallbackFn, 4\n"
    ".extern snd_returnCallbackData, 8\n"
    ".extern snd_batchIndex, 4\n"
    ".extern snd_batchCommandBuffers, 8\n"
    ".extern snd_streamBuffers, 8\n"
    ".extern snd_batchReturnBuffers, 8\n"
    ".extern snd_batchBusy, 4\n"
    ".extern snd_cdCallbackPending, 4\n"
    ".extern snd_cdCallbackFn, 4\n"
    ".extern snd_cdSyncPending, 4\n"
    ".extern snd_cdStreamEndPending, 4\n"
    ".extern snd_cdStatusCallback, 4\n");

typedef struct {
    SndCompleteProc done;
    u64 u_data;
} SndCommandReturnDef;

// Sentinel stored in snd_cdCallbackArg when no CD callback argument is set.
#define SND_CD_CALLBACK_ARG_NONE 0xFFFFFFFF

extern int* snd_currentBuffer;
extern int snd_returnCallbackPending;
extern int* snd_batchCommandBuffers[2];
extern SndCommandReturnDef* snd_streamBuffers[2];
extern int* snd_batchReturnBuffers[2];
extern int snd_batchIndex;
extern int snd_batchBusy;
extern int snd_cdCallbackPending;
extern unsigned int snd_cdCallbackArg;
extern SndCompleteProc snd_cdCallbackFn;
extern u64 snd_cdCallbackData;
extern int snd_cdSyncPending;
extern int snd_cdStreamEndPending;
extern SndCdCallback snd_cdStatusCallback;
extern unsigned int snd_iopReturnWord __attribute__((section(".data")));
extern SndCompleteProc snd_returnCallbackFn;
extern u64 snd_returnCallbackData;
extern int snd_GotReturns(void);
extern void snd_SendCurrentBatch(void);
extern int snd_StreamSafeCdSync(int);
extern void FlushCache(int);

int snd_FlushSoundCommands(void) {
    int which;
    int i;
    u64 data;
    SndCompleteProc fn;
    unsigned int arg_val;

    if (snd_currentBuffer != 0 && snd_GotReturns() != 0) {
        if (snd_returnCallbackPending != 0) {
            if (snd_returnCallbackFn != 0) {
                snd_returnCallbackFn(snd_iopReturnWord, snd_returnCallbackData);
            }
            snd_returnCallbackFn = 0;
            snd_returnCallbackPending = 0;
        }
        else {
            which = snd_batchIndex != 1;
            i = 0;
            if (*snd_batchCommandBuffers[which] > 0) {
                while (1) {
                    if (snd_streamBuffers[which][i].done != 0) {
                        snd_streamBuffers[which][i].done(
                            snd_batchReturnBuffers[which][i + 1],
                            snd_streamBuffers[which][i].u_data);
                    }
                    i++;
                    if (*snd_batchCommandBuffers[which] <= i) {
                        break;
                    }
                }
            }
        }
    }
    if (snd_cdCallbackPending != 0) {
        FlushCache(0);
        arg_val = snd_cdCallbackArg;
        if (arg_val != SND_CD_CALLBACK_ARG_NONE) {
            fn = snd_cdCallbackFn;
            if (fn != 0) {
                // Barriers pin the data load/clear in the macro region so the
                // arg clear takes the callback-test delay slot (beql).
                asm volatile("" : : : "memory");
                data = snd_cdCallbackData;
                snd_cdCallbackFn = 0;
                snd_cdCallbackData = 0;
                asm volatile("" : : : "memory");
                fn(arg_val, data);
            }
            snd_cdCallbackArg = 0;
            snd_cdCallbackPending = 0;
        }
    }
    if (snd_currentBuffer == 0 &&
        *snd_batchCommandBuffers[snd_batchIndex] != 0 &&
        snd_batchBusy == 0) {
        snd_SendCurrentBatch();
    }
    if (snd_cdSyncPending != 0) {
        snd_StreamSafeCdSync(1);
        if (snd_cdStreamEndPending != 0) {
            snd_cdSyncPending = 0;
            snd_cdStreamEndPending = 0;
            if (snd_cdStatusCallback != 0) {
                snd_cdStatusCallback(1);
            }
        }
    }
    return snd_currentBuffer != 0 || snd_cdCallbackPending != 0;
}
