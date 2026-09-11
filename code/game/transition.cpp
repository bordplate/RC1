#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", Transition_DrawSky__Fv);

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
