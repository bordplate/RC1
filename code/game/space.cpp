#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/space", FUN_0022de10_rename);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_0022DF40);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_0022E180);

extern int spaceLoadPending;
// Match-sensitive: these stay PLAIN externs (no .data section attribute).
// EGC emits one store pseudo per access, and ps2eeas — which sees each
// reference before the end-of-file .extern — expands it in place to the
// original's lui at / sw pair. A .data attribute makes EGC split each
// address into two schedulable lui instructions, which reorders the stores
// and changes the base registers.
//
// Space id requested by space_beginLoad; consumed by DoSpaceTransition.
extern int spaceLoadId;
// Set when a space load is requested, cleared by the level init.
extern int spaceLoadInProgress;

void space_beginLoad(int loadId) {
    spaceLoadId = loadId;
    spaceLoadPending = 1;
    spaceLoadInProgress = 1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_0022E1A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_0022E420);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_0022E8C8);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_0022EA08);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_0022EAA8);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_0022F288);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_0022F5B0);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_0022F778);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_00230EE8);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_00230F60);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_00231608);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_002316E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_00231878);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_00231BD8);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", DoSpaceTransition);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_002327A0);
