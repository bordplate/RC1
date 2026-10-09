#include "common.h"

// Sky-draw setup callees (defined in skyfunc.cpp) and the VU1 GS-register
// appender (defined in draw_post_post.cpp).
void SetupSkyGifPaging();
// Symbol override: the ELF is stripped, so the callee's real cfront name is
// unknown; the symbols.txt label for this skyfunc.cpp function is the
// un-mangled placeholder SkyLevelGeneric___maybe, which a natural
// `void SkyLevelGeneric()` (mangling to ...__Fv) would not reference.
void SkyLevelGeneric() asm("SkyLevelGeneric___maybe");
void DoSkyGifPaging();
// Symbol override: the original calls it with only the register and value; the
// third bool parameter of the exported symbol is never materialized at any
// call site, so the 2-param signature mangles to ...__FUiUl, not the
// required ...__FUiUlb.
void VU1_addGSregister(unsigned int reg, unsigned long value)
    asm("VU1_addGSregister__FUiUlb");

// frameBufferBase must be a .data symbol here: with this TU's
// -mno-split-addresses a plain in-window extern would lower to a GP-relative
// load, but the original reads it with a self-based absolute (lui/lw).
extern unsigned int frameBufferBase __attribute__((section(".data")));

// GS register numbers and values programmed during sky-draw setup. The
// register offsets are hardware-defined and unnamed in the available references
// (same treatment as the VU1_*_GS_REG / OCCL_DEBUG_* constants in
// draw_post_post.cpp). The 0x4e write reprograms the same frame-buffer-base
// register (and value form) the occlusion overlay uses there.
#define SKY_SETUP_GS_REG 0x47
#define SKY_SETUP_GS_VALUE 0x5360b
#define SKY_OVERLAY_GS_REG 0x4e
#define SKY_OVERLAY_FB_SHIFT 13
#define SKY_OVERLAY_VRAM_BASE 0x1000000

// Draws the sky level: set up the sky GIF pages, draw the generic sky, run the
// GIF paging, then program the two GS registers that select the sky texture and
// its frame-buffer-derived base.
void Transition_DrawSky() {
    SetupSkyGifPaging();
    SkyLevelGeneric();
    DoSkyGifPaging();
    VU1_addGSregister(SKY_SETUP_GS_REG, SKY_SETUP_GS_VALUE);
    VU1_addGSregister(SKY_OVERLAY_GS_REG,
                      (int)frameBufferBase >> SKY_OVERLAY_FB_SHIFT
                          | SKY_OVERLAY_VRAM_BASE);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", func_001E9B10);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", Transition_LoadWad);

struct HelpMsg;
extern struct HelpMsg* HelpMsgs;

// Transition_LoadWad loads two pointer globals before calling Help_LoadMsgs:
// the help-message data base and the per-set offset table. Both live inside the
// gp window, so a plain extern would compile to GP-relative loads; the original
// uses absolute (lui/lw) loads, which the .data section attribute forces. With
// the TU's -mno-split-addresses each load becomes a single self-based lui/lw,
// matching the original's single-register absolute pattern.
extern char* helpMsgData     __attribute__((section(".data")));
extern int*  helpOffsetTable __attribute__((section(".data")));
extern int   HelpMsgCount    __attribute__((section(".data")));

// Loads the help message set: node = base + offsetTable[set], then stores the
// set's message count in HelpMsgCount and the entry array base (node + 8) in
// HelpMsgs.
void Help_LoadMsgs(int set) {
    int base = (int)helpMsgData;
    int offset = helpOffsetTable[set];
    char* node = (char*)base + offset;

    // Original count store: a hoisted "lui $6,0x1A0000" followed by
    // "sw count,%lo(HelpMsgCount)($6)". 0x1A0000 is HelpMsgCount's (0x1996FC)
    // high-16 page. EGC only reproduces this split with the page pinned to $6
    // (+ barrier) and the count in $4, under -fno-schedule-insns and
    // -mno-split-addresses.
    register int helpCountPage asm("$6") = 0x1A0000;
    asm volatile("" : "+r"(helpCountPage));
    register int count asm("$4") = *(int*)node;
    char* next = node + 8;
    asm volatile("sw %0,%%lo(HelpMsgCount)(%1)" : : "r"(count), "r"(helpCountPage));
    HelpMsgs = (struct HelpMsg*)next;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", Transition_UpdateMovieCamera__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", Transition_FUN_001eb0a8);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", Transition_DefaultDraw__Fb);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", func_001EB740);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", Transition_DoTransition__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", func_001EBC88);
