#include "common.h"
#include "types.h"

typedef void (*SndCompleteProc)(int, u64);

extern int snd_cdBankLoadRequest __attribute__((section(".data")));
extern SndCompleteProc snd_cdCallbackFn;
extern unsigned int snd_cdCallbackArg;
extern u64 snd_cdCallbackData;
extern int snd_cdRpcServer __attribute__((section(".data")));
extern int snd_cdLoadError;
extern int snd_cdCallbackPending;
extern int snd_StreamSafeCdSync(int);
extern void snd_FlushSoundCommands(void);
extern void FlushCache(int);
extern int sceSifCheckStatRpc(void*);
extern int sceSifCallRpc(void*, int, int, void*, int, void*, int, void (*)(void*), void*);
extern void func_00116078(void*);
extern int snd_NonIdleErrorString __attribute__((section(".data")));
extern int snd_BankLoadFromEEProgressString __attribute__((section(".data")));
extern int snd_BankLoadFromEECdBusyString __attribute__((section(".data")));

/* ps2eeas is single-pass: these two must be known as small data before their
 * normal-order references so they expand GP-relative, while arg/data (unseeded)
 * expand absolute. */
asm(".extern snd_cdCallbackPending, 4\n"
    ".extern snd_cdCallbackFn, 4");

// "No result yet" sentinel preloaded into snd_cdCallbackArg before the CD
// bank-load RPC; the unsigned spelling (not -1) is required: EGC emits the
// original's lui/ori constant split only for the unsigned form.
#define SND_CD_CALLBACK_ARG_NONE 0xFFFFFFFF

// Asks the CD RPC server to load a sound bank that lives in EE memory: arms
// the CD callback (snd_FlushSoundCommands fires cb with the IOP-written
// result word and user_data once the result leaves the sentinel) and issues
// the 0x57 bank-load RPC. Reports an error string and returns without
// loading when a load is already pending or the CD is still streaming.
void snd_BankLoadFromEE_CB(void* ee_loc, SndCompleteProc cb, u64 user_data) {
    snd_cdLoadError = 0;
    if (snd_cdCallbackPending != 0) {
        func_00116078(&snd_BankLoadFromEEProgressString);
        return;
    }
    if (snd_StreamSafeCdSync(1) == 1) {
        func_00116078(&snd_BankLoadFromEECdBusyString);
        return;
    }
    snd_cdBankLoadRequest = (int)ee_loc;
    snd_cdCallbackArg = SND_CD_CALLBACK_ARG_NONE;
    snd_cdCallbackFn = cb;
    snd_cdCallbackData = user_data;
    while (sceSifCheckStatRpc(&snd_cdRpcServer) != 0) {
        func_00116078(&snd_NonIdleErrorString);
        snd_FlushSoundCommands();
        FlushCache(0);
    }
    snd_cdCallbackPending = 1;
    sceSifCallRpc(&snd_cdRpcServer, 0x57, 1, &snd_cdBankLoadRequest, 4,
                  &snd_cdCallbackArg, 4, 0, 0);
}
