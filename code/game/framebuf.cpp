#include "camera.h"
#include "common.h"
#include "sce_gs.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FA860);

// Unreachable dead tail the original compiler emitted after
// func_001FA860 (FastMapMaskRLE, blocked: handwritten trapping signed
// arithmetic, see decomp_state/notes/framebuf_func_001FA860.md): four
// stack-deallocation `addiu sp,sp,N` (0xE0, 0x70, 0x80, 0x90) with a nop
// after each of the first three, plus the alignment nop before
// SetupFS_AA_buffer. The unit sizes do not match the parent's frame, as in
// the documented multi-unit dead addiu-sp tail family. Nothing reaches it
// (no Ghidra function; tools/deadness_scan.py: 0 references), so the bytes
// are preserved with raw asm per the dead-tail policy; the parent keeps its
// INCLUDE_ASM.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001FA958, 0x1C\n"
    "glabel func_001FA958\n"
    "    .word 0x27BD00E0\n"
    "    .word 0x00000000\n"
    "    .word 0x27BD0070\n"
    "    .word 0x00000000\n"
    "    .word 0x27BD0080\n"
    "    .word 0x00000000\n"
    "    .word 0x27BD0090\n"
    "endlabel func_001FA958\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", SetupFS_AA_buffer__Fiiiiii);

extern s64 frameBufferColor __attribute__((section(".data")));

void SetBackgroundColor(s32 r, s32 g, s32 b) {
    frameBufferColor = (long)r | ((long)g << 8) | ((long)b << 0x10) | (0x8000ULL << 0x10);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", PutDispBuffer__Fv);

// VIF packet tags for the data-reference record appended below (same
// values vuchain.cpp uses for its chain appenders). The low byte (qcnt) is
// the streamed record's size in 16-byte units.
#define VU1_DATA_REF_TAG 0x30000000
#define VU1_DATA_REF_END_TAG 0x50000000
// 16-byte unit count (qcnt) of the large draw env GIF record that
// SetupFS_AA_buffer builds at OcclCamParamBlock+0x30.
#define DRAW_ENV_LARGE_QCNT 9
// 16-byte unit count (qcnt) of the small draw env GIF record that
// SetupFS_AA_buffer builds at OcclCamParamBlock+0xC0.
#define DRAW_ENV_SMALL_QCNT 9
// 16-byte unit count (qcnt) of the AA clear-black register block that
// SetupFS_AA_buffer builds at aaClearBlackRegs.
#define AA_CLEAR_BLACK_QCNT 0x15
// 16-byte unit count (qcnt) of the AA-blur clear register block that
// AA_BlurPass references at aaClearRegs.
#define AA_BLUR_CLEAR_QCNT 0x26
// 16-byte unit count (qcnt) of the AA display register block that
// framebuf_appendSmallSetup references at aaDisplayRegs.
#define AA_DISPLAY_QCNT 0x29
// The original carries this no-op address mask on the giftag pointer; it is
// what EGC lowers to the zero-extend `and r, r, -1` the original emits
// before the store, so the mask must stay.
#define AA_GIFTAG_ADDR_MASK 0xFFFFFFF

// Head of the VU1 command chain. The double volatile forces EGC to re-load
// the head before each packet store, and the plain (non-.data) declaration
// keeps each load a bare self-based pseudo the SN assembler expands in
// place; the final store must go through the non-.data alias so ps2eeas
// emits it GPREL in the branch delay slot.
extern volatile u32* volatile vu1ChainHead;
extern volatile u32* vu1ChainHeadStore;
// Pointer to the shared frame-buffer parameter block; SetupFS_AA_buffer
// installs &occlCamParamBase here and the buffer-setup/append functions read
// the GS environment blocks off it.
extern OcclCamParamBlock* aaBuffPtr;

// GS GIF stream that SetupFS_AA_buffer builds as the AA pass's clear-to-black
// register payload; appendClearBlackDataRef streams it through the VU1 chain.
extern u32 aaClearBlackRegs[AA_CLEAR_BLACK_QCNT * 4];
// GS register block the AA blur pass streams through the VU1 chain;
// AA_BlurPass appends a data-reference record pointing at it.
extern u32 aaClearRegs[AA_BLUR_CLEAR_QCNT * 4];
// GS register block the small-setup pass streams through the VU1 chain;
// framebuf_appendSmallSetup appends a data-reference record pointing at it.
extern u32 aaDisplayRegs[AA_DISPLAY_QCNT * 4];

// Append the large draw-env VIF data-reference record to the VU1 command
// chain so the VU streams it later, or upload the draw env to the GS
// directly when the chain is not running.
void PutDrawBufferLarge() {
    volatile u32* head = vu1ChainHead;
    if (head) {
        head[0] = VU1_DATA_REF_TAG | DRAW_ENV_LARGE_QCNT;
        vu1ChainHead[1] = (u32)&aaBuffPtr->giftagDrawLarge & AA_GIFTAG_ADDR_MASK;
        vu1ChainHead[2] = 0;
        vu1ChainHead[3] = VU1_DATA_REF_END_TAG | DRAW_ENV_LARGE_QCNT;
        vu1ChainHeadStore = vu1ChainHead + 4;
    }
    else {
        sceGsPutDrawEnv(&aaBuffPtr->giftagDrawLarge);
    }
}

// Append the AA clear-black register block to the VU1 command chain as a VIF
// data-reference record so the VU streams it to the GS later; a no-op when
// the chain is not running.
void appendClearBlackDataRef() {
    volatile u32* head = vu1ChainHead;
    if (head) {
        head[0] = VU1_DATA_REF_TAG | AA_CLEAR_BLACK_QCNT;
        vu1ChainHead[1] = (u32)aaClearBlackRegs;
        vu1ChainHead[2] = 0;
        vu1ChainHead[3] = VU1_DATA_REF_END_TAG | AA_CLEAR_BLACK_QCNT;
        vu1ChainHead = vu1ChainHead + 4;
    }
}

// Append the small draw-env VIF data-reference record to the VU1 command
// chain so the VU streams it later. Unlike PutDrawBufferLarge there is no
// direct-upload fallback: the callers push the chain first and call this
// between PutDrawBufferLarge and appendClearBlackDataRef.
void PutDrawBufferSmall() {
    volatile u32* head = vu1ChainHead;
    head[0] = VU1_DATA_REF_TAG | DRAW_ENV_SMALL_QCNT;
    vu1ChainHead[1] = (u32)&aaBuffPtr->giftagDrawSmall & AA_GIFTAG_ADDR_MASK;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | DRAW_ENV_SMALL_QCNT;
    vu1ChainHeadStore = vu1ChainHead + 4;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB440);

// Append the AA-blur clear register block to the VU1 command chain as a VIF
// data-reference record so the VU streams it to the GS later.
void AA_BlurPass() {
    volatile u32* head = vu1ChainHead;
    head[0] = VU1_DATA_REF_TAG | AA_BLUR_CLEAR_QCNT;
    vu1ChainHead[1] = (u32)aaClearRegs;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | AA_BLUR_CLEAR_QCNT;
    vu1ChainHeadStore = vu1ChainHead + 4;
}

// Append the AA display register block to the VU1 command chain as a VIF
// data-reference record so the VU streams it to the GS later.
void framebuf_appendSmallSetup() {
    volatile u32* head = vu1ChainHead;
    head[0] = VU1_DATA_REF_TAG | AA_DISPLAY_QCNT;
    vu1ChainHead[1] = (u32)aaDisplayRegs;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | AA_DISPLAY_QCNT;
    vu1ChainHeadStore = vu1ChainHead + 4;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB740);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB8F0);

// Unreachable dead tail after func_001FB8F0 (0x1FB8F0): one
// `lw v0,20(v1)`; the unit is not a stack deallocate and its value load
// feeds nothing. Nothing reaches 0x1FBAB0 (0 jal/j/branch/data
// references; no Ghidra function), so it is not a function; the original
// compiler emitted this byte after the parent's RTL, so it is preserved
// here as an exact word. The trailing nop pads to the 8-aligned
// mode_freezeInit entry at 0x1FBAB8.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001FBAB0, 0x4\n"
    "glabel func_001FBAB0\n"
    "    .word 0x8c620014\n"
    "endlabel func_001FBAB0\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);
