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

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", framebuf_appendLargeSetup__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", PutDrawBufferSmall__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB440);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", AA_BlurPass__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", framebuf_appendSmallSetup__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB740);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FB8F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/framebuf", func_001FBAB0);
