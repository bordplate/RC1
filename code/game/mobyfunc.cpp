#include "common.h"

#include "mobyfunc.h"

// Plain externs (no section attribute): in-window .bss globals that the
// original loads as self-based lui/lw pairs, which the -G8 bare small-data
// pseudo produces. A section attribute would force a two-register split load.
extern MobyInstance* MobyInstanceEnd;
extern MobyInstance* MobyInstancePermEnd;
extern s32 MobyVars;
extern s32 numSpawnableMobys;
extern s32 worldUpdateTime;

#if !defined(SKIP_ASM) && !defined(ALLOW_NONMATCHING)
INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", CreateMoby__Fi);
#endif

#if defined(SKIP_ASM) || defined(ALLOW_NONMATCHING)
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
            moby += 1;
            goto block_13;
        }
        if ((u32)worldUpdateTime < (u64)moby->unk1) {
            moby += 1;
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

// MobyInstance->state stamps written by DeleteMoby: 0xFD for a pool instance
// (moby < MobyInstanceEnd), 0xFE for a permanent instance at or past it. The
// CreateMoby scan skips slots with state < 0xFE, so only 0xFE/0xFF slots are
// spawnable.
#define MOBY_STATE_POOL_DELETED 0xFD
#define MOBY_STATE_PERM_DELETED 0xFE

// Stamp DeleteMoby hands to UpdateMobyGrids to retire the moby's grid cells:
// the handwritten grid walk stores it at MobyInstance+0xA0 and stops matching
// cells whose 0xAC stamp equals it.
#define MOBY_GRID_DELETE_STAMP 0x80807F7F

// Deleted slots stamp unk1 (last-update time) with worldUpdateTime + 2.
// CreateMoby's spawn scan skips slots whose unk1 is ahead of worldUpdateTime,
// so the offset keeps a freshly deleted slot out of the scan.
#define MOBY_DELETE_TIMESTAMP_OFFSET 2

// C linkage: this is a handwritten assembly entry point in game/mobyproc;
// the boot-ELF symbol is unmangled and the grid walk calls itself through it.
extern "C" void UpdateMobyGrids(MobyInstance* moby, u32 stamp);

// The moby-deletion entry point at 0x20C828 is the unmangled symbol DeleteMoby
// in the boot ELF; a C++ free function here would mangle differently, so pin
// it with a symbol override instead of assuming C linkage.
void DeleteMoby(MobyInstance* moby) asm("DeleteMoby");
void DeleteMoby(MobyInstance* moby) {
    if ((u32)moby < (u32)MobyInstanceEnd) {
        moby->state = MOBY_STATE_POOL_DELETED;
    } else {
        moby->state = MOBY_STATE_PERM_DELETED;
    }

    moby->unk1 = worldUpdateTime + MOBY_DELETE_TIMESTAMP_OFFSET;
    UpdateMobyGrids(moby, MOBY_GRID_DELETE_STAMP);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020C880);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020C940);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020C9D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020CAD8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", AttachManipulator);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", DetachManipulator);

// Pool of 16 moby render slots; each holds a MobyInstance*. A per-slot vec4
// table follows the pool in memory. MobyAssignRenderSlot claims one slot.
#define MOBY_RENDER_SLOT_COUNT 16
extern MobyInstance* mobyRenderSlots[MOBY_RENDER_SLOT_COUNT];

// Claims (or reuses) a render slot for the moby: the first slot that is
// empty or already holds the moby, storing the moby there and returning the
// slot index, or -1 if every slot is held by another moby.
// Symbol override: the boot ELF is stripped, so the original name is unknown
// and the still-assembly callers link against Splat's func_0020CC18
// placeholder, which natural C++ mangling would not produce.
int MobyAssignRenderSlot(MobyInstance* m) asm("func_0020CC18");
int MobyAssignRenderSlot(MobyInstance* m)
{
    int count;
    for (count = 0; count < MOBY_RENDER_SLOT_COUNT; count++) {
        MobyInstance** p = mobyRenderSlots + count;
        if (*p == 0 || *p == m) {
            *p = m;
            return count;
        }
    }
    return -1;
}

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

// VU1 moby animation chain head cell; the writer (VU1_swapChain) is
// unmatched, so the address name is retained.
extern int* D_0015F638;

// VU1 swap-chain slot pointer maintained by VU1_initChain / VU1_swapChain
// (equals the chain head pointer minus 0x2000); the writers are unmatched,
// so the address name is retained.
extern int* D_0015F63C;

void ProcessMobyAnimData() {
    FlushCache(0);
    // 0x70003800 is a DMC destination constant with no original symbol; a
    // named symbol changes EGC's lui/ori setup order and breaks the match.
    FastMemCopy((void*)0x70003800, D_00165500, 0x800);
    MobyAnimProc(D_0015F638, D_0015F63C);
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
extern int mobyAnimHead;
// Base of the 0x100-byte MobyInstance array; InitMobyInstance derives the
// instance index as (instance - base) >> 8 and MobyProc indexes it the same
// way. The boot-ELF writers are level code, so this value is only read here.
extern int mobyInstanceBase;
// Same object as vu1ChainHead (vuchain.cpp), declared plain so the head
// value is read as an int for the overflow check below.
extern int vu1ChainHead;
// VU1 chain overflow limit for this frame's moby packets; DrawMobysSetup
// writes D_0015F63C - 0x10000 here before the check.
extern int vu1ChainLimit;
// C linkage: MobyProc is a handwritten assembly entry point referenced by the
// generated assembly in this file; the linker pins it at 0x00211808.
extern "C" int MobyProc(int, int, int, int);
extern char vuChainOverflowMessage[];
// C linkage: this diagnostic entry point is supplied by generated sce/lib.s.
extern "C" void STUB_printf(const char* fmt, ...);
void DrawMobysSetup();
// C linkage: the original symbol is unmangled in the boot ELF.
extern "C" void DrawMobysCleanUp();

// C linkage: the original symbol is unmangled.
extern "C" void DrawMobys() {
    DrawMobysSetup();
    if (D_0018A2D8[0] != 0) {
        InitMobyClassDists();
        int ret = MobyProc(mobyInstanceBase, mobyAnimHead, -1, 1);
        mobyAnimHead = ret;
        if (vu1ChainHead > vu1ChainLimit)
            STUB_printf(vuChainOverflowMessage);
    }
    DrawMobysCleanUp();
}

// Unreachable dead tail after DrawMobys (0x20D4E0): one `addiu sp,sp,0x70`
// followed by three nops. No nearby function uses a 0x70 frame, so the unit
// does not belong to any live function; nothing reaches 0x20D4E0 (0 jal/j/
// branch/data references). The original compiler emitted these bytes after the
// preceding function's RTL, so they are preserved as exact words. (See the
// 989snd dead addiu-sp tail family.)
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_0020D4E0, 0x10\n"
    "glabel func_0020D4E0\n"
    "    .word 0x27bd0070\n"
    "    .word 0x00000000\n"
    "    .word 0x00000000\n"
    "    .word 0x00000000\n"
    "endlabel func_0020D4E0\n"
    "    .set reorder\n"
    "    .set at\n"
);

// Bit positions of each field within the packed moby lights word (m->unk1):
//   (high << MOBY_LIGHTS_HI_SHIFT) | lo0 | (lo1 << MOBY_LIGHTS_LO1_SHIFT)
//   | (lo2 << MOBY_LIGHTS_LO2_SHIFT)
#define MOBY_LIGHTS_HI_SHIFT  0x20
#define MOBY_LIGHTS_LO1_SHIFT 8
#define MOBY_LIGHTS_LO2_SHIFT 0x10

// Packs the four fields into the moby's 64-bit lights word (m->unk1) and
// returns the packed value.
//
// The register pin and per-shift barriers reproduce the original's
// register allocation and schedule: the high field's shift stays in-place in
// a1 (rather than being folded into the v0 accumulator), all three shifts
// precede the first or, and the or-chain accumulates into v0.
u64 SetMobyLights(MobyInstance* m, u64 high, u64 lo0, u64 lo1, u64 lo2) {
    register u64 highS asm("$5") = high;
    highS <<= MOBY_LIGHTS_HI_SHIFT;
    asm volatile("" : "+r"(highS));
    lo1 <<= MOBY_LIGHTS_LO1_SHIFT;
    asm volatile("" : "+r"(lo1));
    lo2 <<= MOBY_LIGHTS_LO2_SHIFT;
    asm volatile("" : "+r"(lo2));
    u64 lights = highS | lo0 | lo1 | lo2;
    m->unk1 = lights;
    return lights;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020D510);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", func_0020D580);
