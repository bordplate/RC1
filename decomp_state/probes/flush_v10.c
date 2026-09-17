#include "common.h"
#include "types.h"

extern void* snd_currentBuffer;
extern int snd_batchIndex;
extern int snd_batchBusy;
extern int* snd_batchCommandBuffers[2];
extern int* snd_batchReturnBuffers[2];
extern int* snd_streamBuffers[2];
extern int snd_returnCallbackPending;
extern void (*snd_returnCallbackFn)(int, u64);
extern u64 snd_returnCallbackData;
extern int snd_iopReturnWord __attribute__((section(".data")));
extern int snd_cdCallbackPending;
extern void (*snd_cdCallbackFn)(int, u64);
extern u64 snd_cdCallbackData __attribute__((section(".data")));
extern int snd_cdCallbackArg __attribute__((section(".data")));
/* Same object as snd_cdCallbackArg, declared plain so the pre-call clear is
 * a single GPREL16 store, as in the original. */
extern int snd_cdCallbackArgGp;
/* Distinct .data aliases so the post-call clears materialize their own
 * self-based base instead of CSE-ing the load's base across the call. */
extern int snd_cdCallbackArgClear __attribute__((section(".data")));
extern u64 snd_cdCallbackDataClear __attribute__((section(".data")));
extern int snd_cdSyncPending;
extern int snd_cdStreamEndPending;
extern int snd_cdStatusCallback;
extern int snd_GotReturns(void);
extern void snd_SendCurrentBatch(void);
extern int snd_StreamSafeCdSync(int arg);
typedef void (*SndStatusCallbackFn)(int);

typedef struct {
    void (*fn)(int, u64);
    int field_04;
    u64 data;
} SndStreamEntry;

int snd_FlushSoundCommands(void) {
    if (snd_currentBuffer != 0) {
        if (snd_GotReturns() != 0) {
            if (snd_returnCallbackPending != 0) {
                if (snd_returnCallbackFn != 0)
                    snd_returnCallbackFn(snd_iopReturnWord, snd_returnCallbackData);
                snd_returnCallbackFn = 0;
                snd_returnCallbackPending = 0;
            } else {
                int idx = snd_batchIndex != 1;
                int i;
                for (i = 0; i < *snd_batchCommandBuffers[idx]; i++) {
                    SndStreamEntry* entry = (SndStreamEntry*)snd_streamBuffers[idx] + i;
                    if (entry->fn)
                        entry->fn(snd_batchReturnBuffers[idx][i + 1], entry->data);
                }
            }
        }
    }
    if (snd_cdCallbackPending != 0) {
        int arg;

        FlushCache(0);
        arg = snd_cdCallbackArg;
        if (arg != 0xFFFFFFFF) {
            register void (*callback)(int, u64) asm("$2");

            callback = snd_cdCallbackFn;
            if (callback != 0) {
                u64 data;

                snd_cdCallbackArgGp = 0;
                data = snd_cdCallbackData;
                snd_cdCallbackFn = 0;
                snd_cdCallbackDataClear = 0;
                callback(arg, data);
                snd_cdCallbackArgClear = 0;
            }
            snd_cdCallbackPending = 0;
        }
    }
    if (snd_currentBuffer == 0) {
        if (*snd_batchCommandBuffers[snd_batchIndex] != 0 && snd_batchBusy == 0)
            snd_SendCurrentBatch();
    }
    if (snd_cdSyncPending != 0) {
        snd_StreamSafeCdSync(1);
        if (snd_cdStreamEndPending != 0) {
            snd_cdSyncPending = 0;
            if (snd_cdStatusCallback != 0) {
                snd_cdStreamEndPending = 0;
                ((SndStatusCallbackFn)snd_cdStatusCallback)(1);
            }
        }
    }
    return (snd_currentBuffer != 0) || (snd_cdCallbackPending != 0);
}
