#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/space", FUN_0022de10_rename);

INCLUDE_ASM("code/_generated/nonmatchings/game/space", func_0022DF40);

// Unreachable dead tail after func_0022DF40 (0x22DF40, 0x170 frame): one
// `addiu sp,sp,0x10` (word 0x27BD0010) plus a trailing nop at 0x22E180. The
// 0x10 unit size does not match the parent's 0x170 frame (see the 989snd
// dead addiu-sp tail family, 989snd_func_0012E078.md). Nothing reaches
// 0x22E180 (0 jal/j/branch/data references; no Ghidra function), so it is
// not a function; the original compiler emitted this byte after the
// parent's RTL, so it is preserved here as an exact word. Precedent:
// code/game/actuator.cpp dead tails at 0x1E9120 and 0x1E9148.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_0022E180, 0x4\n"
    "glabel func_0022E180\n"
    "    .word 0x27BD0010\n"
    "endlabel func_0022E180\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

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
