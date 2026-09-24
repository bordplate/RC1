#include "common.h"
#include "types.h"
#include "989snd_iop.h"

extern int snd_cdLoadError;
extern int snd_cdCallbackPending;
extern int snd_cdBankLoadRequest[2] __attribute__((section(".data")));
extern unsigned int snd_cdCallbackArg;
extern unsigned int snd_cdCallbackArgGp;
extern unsigned int snd_cdCallbackArgGpRet;
extern int snd_cdRpcServer __attribute__((section(".data")));
extern int snd_BankLoadByLocInProgressErrorString __attribute__((section(".data")));
extern int snd_BankLoadByLocCDBusyErrorString __attribute__((section(".data")));
extern int snd_BankLoadByLocRpcErrorString __attribute__((section(".data")));
extern int snd_NonIdleErrorString __attribute__((section(".data")));

extern int snd_StreamSafeCdSync(int);
extern int sceSifCheckStatRpc(void*);
extern int sceSifCallRpc(void*, int, int, void*, int, void*, int, void (*)(void*), void*);
/* The generated SCE library provides this SIF RPC wrapper. Its exact SDK API
 * identity is unresolved, so retain the address-based name in this C TU. */
extern void func_00116078(void*);
extern int snd_FlushSoundCommands(void);
extern void FlushCache(int);

/* ps2eeas is single-pass: these must be known as small data before their
 * normal-order references so they expand GP-relative, while
 * snd_cdCallbackArg (unseeded) expands absolute. */
asm(".extern snd_cdLoadError, 4\n"
    ".extern snd_cdCallbackPending, 4\n"
    ".extern snd_cdCallbackArgGp, 4\n"
    ".extern snd_cdCallbackArgGpRet, 4");

// "No result yet" sentinel preloaded into snd_cdCallbackArg before the CD
// bank-load RPC; the unsigned spelling (not -1) is required: EGC emits the
// original's lui/ori constant split only for the unsigned form.
#define SND_CD_CALLBACK_ARG_NONE 0xFFFFFFFF

// Recorded into snd_cdLoadError when the CD bank-load RPC call itself fails.
// The code's meaning is unresolved; Deadlocked writes the same 0x106 into
// gLocalLoadError at the same failure point.
#define SND_CD_LOAD_ERROR_RPC_FAILURE 0x106

// snd_cdCallbackArg is the one 4-byte word at 0x15ED00 the IOP writes the CD
// callback result into. The original accesses it through three names: the
// unseeded name expands self-based absolute under ps2eeas (the sentinel
// store, the address taken for the RPC argument, and the polling loop load),
// while two seeded GP-relative aliases (config/linker_aliases.ld) supply the
// two GPREL loads issued after the RPC. The test and the return must use
// separate aliases: with a single name EEGCC CSEs the return load into a
// move, but the original reloads the word in the bne delay slot.
// This TU is assembled by ps2eeas (not the GNU assembler used by
// 989snd_mid.o) because the self-based absolute expansion of the unseeded
// references only the SN assembler produces.
int snd_BankLoadByLoc(int loc, int offset) {
    snd_cdLoadError = 0;
    if (snd_cdCallbackPending != 0) {
        func_00116078(&snd_BankLoadByLocInProgressErrorString);
        return 0;
    }
    if (snd_StreamSafeCdSync(SND_CD_SYNC_MODE_CHECK) == 1) {
        func_00116078(&snd_BankLoadByLocCDBusyErrorString);
        return 0;
    }
    snd_cdBankLoadRequest[1] = offset;
    snd_cdCallbackArg = SND_CD_CALLBACK_ARG_NONE;
    snd_cdBankLoadRequest[0] = loc;
    while (sceSifCheckStatRpc(&snd_cdRpcServer) != 0) {
        func_00116078(&snd_NonIdleErrorString);
        snd_FlushSoundCommands();
        FlushCache(0);
    }
    if (sceSifCallRpc(&snd_cdRpcServer, SND_IOP_CMD_CD_BANK_LOAD_BY_LOC, 1,
                      &snd_cdBankLoadRequest, 8, &snd_cdCallbackArg, 4, 0, 0) < 0) {
        func_00116078(&snd_BankLoadByLocRpcErrorString);
        snd_cdLoadError = SND_CD_LOAD_ERROR_RPC_FAILURE;
        return 0;
    }
    if (snd_cdCallbackArgGp != -1)
        return snd_cdCallbackArgGpRet;
    do
        FlushCache(0);
    while (snd_cdCallbackArg == -1);
    return snd_cdCallbackArg;
}

// Unreachable dead tail the original compiler emitted after
// snd_BankLoadByLoc: a 0x60 then a 0x50 stack deallocation plus the
// alignment nop before snd_BankLoadFromEE_CB. EGC 2.95.2 never regenerates a
// dead frame deallocation after the epilogue (probed), so the bytes are
// preserved with raw asm. The .align 3 and the nonmatching/glabel pair
// reproduce the generated assembly's layout (one alignment nop before the
// fragment).
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_0012E078, 0xC\n"
    "glabel func_0012E078\n"
    "    .word 0x27bd0060\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0050\n"
    "endlabel func_0012E078\n"
    "    .set reorder\n"
    "    .set at\n"
);
