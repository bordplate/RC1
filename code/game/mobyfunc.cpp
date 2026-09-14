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
// game/mobyproc; its symbol is not cfront-mangled. It dereferences its first
// argument as a moby animation chain head, so both parameters are pointers.
extern "C" void MobyAnimProc(int* p1, int* p2);

// 0x800-byte VU1 moby animation data buffer; the first 0x80 bytes are also
// DMA'd as a VU1 header by the handwritten lighting pass at 0x234F98. The
// data producer is not decompiled yet, so the address name is retained.
extern u8 D_00165500[];

// VU1 swap-chain slot pointer maintained by VU1_initChain / VU1_swapChain
// (equals the chain head pointer minus 0x2000); the writers are unmatched,
// so the address name is retained.
extern int* D_0015F63C;

// EGC codegen exception: loading the D_0015F638 symbol in any named form
// (scalar or array, with or without .data, with or without
// -mno-split-addresses) makes EGC allocate a separate base register and
// reschedules the GPREL16 load, a 3-word diff against the original's
// self-based `lui a0; lw a0` pair; -mno-split-addresses additionally flips
// the FastMemCopy argument setup order in this function. Only a
// constant-address load reproduces the original; the address is the linker
// symbol D_0015F638 (see notes/mobyfunc_ProcessMobyAnimData__Fv.md for the
// full probe record and the expert/last-resort escalations).
enum { MOBY_ANIM_CHAIN_ADDRESS = 0x0015F638 };

void ProcessMobyAnimData() {
    FlushCache(0);
    // 0x70003800 is a DMC destination constant with no original symbol; a
    // named symbol changes EGC's lui/ori setup order and breaks the match.
    FastMemCopy((void*)0x70003800, D_00165500, 0x800);
    MobyAnimProc(*(int**)MOBY_ANIM_CHAIN_ADDRESS, D_0015F63C);
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

// Per-frame moby processing gate; only read here in the boot ELF (the writer
// is level code, initial .data value 0), so the address name is retained.
// Declared as an array so the value load is an absolute lui v0/lw v1 pair as
// in the original; a scalar extern would be GPREL16 (invalid out of window).
extern int D_0018A2D8[];
// Moby animation chain head: DrawMobysSetup copies the D_0015F638 chain head
// here and MobyProc's return value is stored back here.
extern int D_0015FF14;
// C linkage: MobyProc is a handwritten assembly entry point referenced by the
// generated assembly in this file; the linker pins it at 0x00211808.
extern "C" int MobyProc(int, int, int, int);
extern char vuChainOverflowMessage[];
// C linkage: this diagnostic entry point is supplied by generated sce/lib.s.
extern "C" void STUB_printf(const char* fmt, ...);
void DrawMobysSetup();
// C linkage: the original symbol is unmangled in the boot ELF.
extern "C" void DrawMobysCleanUp();

// EGC codegen exception: the value loads at these in-window addresses must be
// constant-address casts. Named references (scalar GPREL16 or two-register
// base-$v0 loads) do not reproduce the original's self-based `lui r;
// lw r,off(r)` pairs (see notes/mobyfunc_ProcessMobyAnimData__Fv.md). The
// addresses are the linker symbols D_0015FF14 / D_0015FF18 (build/data/
// lit.lit4.s), vu1ChainHead (0x160F00) and its limit slot 0x160F08.
enum {
    MOBY_ANIM_HEAD_ADDR = 0x0015FF14,
    MOBY_ANIM_DATA_ADDR = 0x0015FF18,
    VU1_CHAIN_HEAD_ADDR = 0x00160F00,
    VU1_CHAIN_LIMIT_ADDR = 0x00160F08,
};

// C linkage: the original symbol is unmangled.
extern "C" void DrawMobys() {
    DrawMobysSetup();
    if (D_0018A2D8[0] != 0) {
        InitMobyClassDists();
        int ret = MobyProc(*(int*)MOBY_ANIM_DATA_ADDR, *(int*)MOBY_ANIM_HEAD_ADDR, -1, 1);
        D_0015FF14 = ret;
        if (*(int*)VU1_CHAIN_HEAD_ADDR > *(int*)VU1_CHAIN_LIMIT_ADDR)
            STUB_printf(vuChainOverflowMessage);
    }
    DrawMobysCleanUp();
}

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020D4E0);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020D510);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020D580);
