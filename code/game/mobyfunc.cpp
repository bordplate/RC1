#include "common.h"

#include "mobyfunc.h"

#if !defined(SKIP_ASM) && !defined(ALLOW_NONMATCHING)
INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", CreateMoby__Fi);
#endif

#if defined(SKIP_ASM) || defined(ALLOW_NONMATCHING)
extern MobyInstance* MobyInstanceEnd __attribute__((section(".bss")));
extern MobyInstance* MobyInstancePermEnd __attribute__((section(".bss")));
extern s32 MobyVars __attribute__((section(".bss")));
extern s32 numSpawnableMobys __attribute__((section(".bss")));
extern s32 worldUpdateTime __attribute__((section(".bss")));

void InitMobyInstance(MobyInstance* mobyInstance, int oClass);

MobyInstance* CreateMoby(s32 oClass) {
    MobyInstance* moby;
    u8 nextState;
    void* mobyVars;

    moby = MobyInstanceEnd;
    if ((u32)moby < (u32)MobyInstancePermEnd) {
        nextState = moby->state;
        loop_2:
        if (nextState < 0xFEU) {
            moby += 0x100;
            goto block_13;
        }
        if ((u32)worldUpdateTime < (u64)moby->unk1) {
            moby += 0x100;
            block_13:
            if ((u32)moby < (u32)MobyInstancePermEnd) {
                nextState = moby->state;
                goto loop_2;
            }
            goto block_16;
        }
        if (nextState == 0xFF) {
            moby[1].state = nextState;
        }
        InitMobyInstance(moby, oClass);
        mobyVars = (void*)(MobyVars + (((s32)(moby - (u32)MobyInstanceEnd) >> 8) << 7));
        moby->pVar = mobyVars;
        FastMemSet(mobyVars, 0, 0x80);
        if (numSpawnableMobys != 0) {
            numSpawnableMobys -= 1;
        }
        return moby;
    }
    block_16:
#ifndef SKIP_ASM
    // These padding instructions preserve the original fallback function tail.
    asm("nop");
    asm("nop");
    asm("nop");
#endif


    return 0;
}

#endif


INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", InitMobyInstance__FP12MobyInstancei);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", DeleteMoby);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020C880);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020C940);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020C9D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020CAD8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", AttachManipulator);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", DetachManipulator);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020CC18);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020CC60);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020CCA8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020CD48);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020CDE8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", DmaMobyTextures);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", PatchMobyGifs);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020CFD0);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020D060);

// C linkage: MobyAnimProc is a handwritten assembly entry point in
// game/mobyproc; its symbol is not cfront-mangled.
extern "C" void MobyAnimProc(int p1, int p2);

// 0x800-byte VU1 moby animation data buffer; the first 0x80 bytes are also
// DMA'd as a VU1 header by the handwritten lighting pass at 0x234F98. The
// data producer is not decompiled yet, so the address name is retained.
extern u8 D_00165500[];

// VU1 swap-chain slot offset maintained by VU1_initChain / VU1_swapChain
// (equals the D_0015F638 value minus 0x2000); the writers are unmatched, so
// the address name is retained.
extern int D_0015F63C;

void ProcessMobyAnimData() {
    FlushCache(0);
    FastMemCopy((void*)0x70003800, D_00165500, 0x800);
    // Constant cast required: a named scalar at 0x15F638 would compile to a
    // GPREL16 load, but the original uses an absolute lui/lw.
    MobyAnimProc(*(int*)0x15F638, D_0015F63C);
}

void InitMobyClassDists() {
    FastMemSet((void*)0x70003A00, 0x40000000, 0x380);
}

extern char mobyBackupBuffer[] __attribute__((section(".data")));

void StashMobyClassDists() {
    FastMemCopy(mobyBackupBuffer, (void*)0x70003A00, 0x380);
}

void RestoreMobyClassDists() {
    FastMemCopy((void*)0x70003A00, mobyBackupBuffer, 0x380);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", DrawMobysSetup__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", DrawMobyList);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", DrawMobysCleanUp);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", DrawMobys);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020D4E0);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020D510);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020D580);
