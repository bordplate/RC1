#include "common.h"

extern int* snd_currentBuffer;
extern int snd_currentBufferIndex;
extern void FlushCache(int);
extern int snd_rpcServer __attribute__((section(".data")));
extern int sceSifCheckStatRpc(void*);
/* The generated SCE library provides this SIF RPC wrapper. Its exact SDK API
 * identity is unresolved, so retain the address-based name in this C TU. */
extern void func_00116078(void*);
extern int snd_NoReturnErrorString __attribute__((section(".data")));

// Value held in a return buffer slot before the IOP writes a return into it.
// The unsigned spelling (not -1) is required: EGC emits the original's
// lui/ori constant split only for the unsigned form.
#define SND_RETURN_SLOT_NONE 0xFFFFFFFF

// Unreachable dead tail the original compiler emitted after
// snd_FlushSoundCommands: two 0x10 stack deallocations plus the alignment
// nop before snd_GotReturns. EGC 2.95.2 never regenerates a dead frame
// deallocation after the epilogue (probed; see
// decomp_state/notes/989snd_snd_FlushSoundCommands.md), so the bytes are
// preserved with raw asm.
asm("addiu $sp,$sp,0x10");
asm("nop");
asm("addiu $sp,$sp,0x10");
asm("nop");

// Polls the IOP return buffer armed by snd_PrepareReturnBuffer and returns 1
// when no buffer is armed or the "no return yet" state (both slots holding
// SND_RETURN_SLOT_NONE) is confirmed, else 0: while the RPC is still busy, or
// after reporting the 989snd error string on unexpected slot contents.
// Callers loop on the zero result.
int snd_GotReturns(void) {
    FlushCache(0);
    if (snd_currentBuffer == 0) {
        return 1;
    }
    if (sceSifCheckStatRpc(&snd_rpcServer) != 0) {
        return 0;
    }
    if (*snd_currentBuffer == SND_RETURN_SLOT_NONE &&
        snd_currentBuffer[snd_currentBufferIndex + 1] == *snd_currentBuffer) {
        snd_currentBuffer = 0;
        return 1;
    }
    func_00116078(&snd_NoReturnErrorString);
    return 0;
}

void snd_PrepareReturnBuffer(int* buf, int index) {
    snd_currentBufferIndex = index;
    snd_currentBuffer = buf;
    buf[index + 1] = 0;
    *buf = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_mid", func_0012DF18);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_mid", snd_BankLoadByLoc);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_mid", func_0012E078);
