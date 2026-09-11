#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", Transition_DrawSky__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", func_001E9B10);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", Transition_LoadWad);

struct HelpMsg;
extern struct HelpMsg* HelpMsgs;

// Loads the help message set: node = base + offsetTable[set], then stores the
// set's message count at 0x1996FC and the entry array base (node + 8) in
// HelpMsgs.
void Help_LoadMsgs(int set) {
    int base = *(int*)0x15EF60;
    int offset = ((int*)(*(int*)0x15EF64))[set];
    char* node = (char*)base + offset;

    // EGC only reproduces the original count store under -fno-schedule-insns
    // when the store's page is pinned to $6 with a barrier (hoisted after the
    // base load, base of the lui/lo pair) and the count is pinned to $4.
    // 0x6904 = 0x1A0000 - 0x1996FC (HelpMsgCount).
    register int helpCountPage asm("$6") = 0x1A0000;
    asm volatile("" : "+r"(helpCountPage));
    register int count asm("$4") = *(int*)node;
    *(int*)(helpCountPage - 0x6904) = count;
    HelpMsgs = (struct HelpMsg*)(node + 8);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", Transition_UpdateMovieCamera__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", Transition_FUN_001eb0a8);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", Transition_DefaultDraw__Fb);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", func_001EB740);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", Transition_DoTransition__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/transition", func_001EBC88);
