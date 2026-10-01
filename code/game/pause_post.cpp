#include "common.h"
#include "types.h"
#include "pause.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00218F98);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002191B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002192A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002196B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00219D80);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00219E10);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00219FA0);

typedef struct {
    u8 pad[0x34];
    u32 actionList;
} PauseActionMode;

extern int pauseActionListA[];
extern int pauseActionListB[];

// Set once in InitOnce__Fv from the memory-card save-info sector read at boot:
// (sector[0x33] != 'N'). Also selects the memcard_Update save-data size
// (0x3C00 when set, 0x3C04 when clear).
extern int pauseActionListMode;

// C linkage: this callback is referenced by the original unmangled pause menu
// action-list table.
extern "C" int SetPauseActionList(PauseActionMode* mode) {
    // EGC's named -G0 load allocates a separate base register (lui $v1;
    // lw $v0, off($v1)); the original reuses one register (lui $v0;
    // lw $v0, off($v0)). A bare lw pseudo makes ps2eeas expand the pair
    // in place self-based (no .extern precedes the reference).
    int listMode;
    asm volatile("lw %0, pauseActionListMode" : "=r"(listMode));
    if (listMode != 0)
        mode->actionList = (u32)pauseActionListA;
    else
        mode->actionList = (u32)pauseActionListB;
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021A1E0);

int pause_callbackNoopA(void) {
    return 0;
}

int pause_callbackNoopB(void) {
    return 0;
}

int pause_resetCallbackState(int* p) {
    p[0x11] = -1;
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021A328);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021ABF8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021B1C8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021B6D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021B7A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021B858);

int pause_callbackNoopC(void) {
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021BDA0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", DrawMapScreen);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021C420);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021C4C0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021C7A0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021C9C8);

// Pause-menu post-callback: stage the item's sprite-list pointer array into
// the IOP screen buffer (shipped to IOP RAM 0x70000150 by the card-screen
// setup at 0x226B08).
int pause_stageSpriteLists(PauseSpriteListMode* mode) {
    u32* src = mode->spriteLists;
    u32* dst = pauseIopSpriteLists;
    int count = PAUSE_SPRITE_LIST_COUNT - 1;
    do {
        *dst = *src;
        src++;
        dst++;
        count--;
    } while (count >= 0);
    return 0;
}

extern u8 pauseSpriteListFlag __attribute__((section(".data")));
extern int pauseSpriteListA[];
extern int pauseSpriteListB[];

// Pause menu callback: select one of two sprite-index lists based on a
// dedicated flag and store the chosen list pointer in the menu item's
// PauseSpriteListMode::spriteLists[1] field.
int pause_selectSpriteList(PauseSpriteListMode* mode) {
    // Match-critical: the != 0 direction makes EGC emit the original beqzl
    // layout; the == 0 form emits bnez (a 3-byte branch-direction diff).
    mode->spriteLists[1] =
        (pauseSpriteListFlag != 0) ? (u32)pauseSpriteListA : (u32)pauseSpriteListB;
    return 0;
}

extern int pauseMemoryCardState __attribute__((section(".data")));

int pause_resetMemoryCardState(void) {
    pauseMemoryCardState = -1;
    return 0;
}

// Symbol override: the still-assembly pause sound-slot allocator at 0x00225AC0
// fills the five-entry pause sound table, whose generated entry point is
// unmangled and cannot be produced by natural C++ linkage.
extern void pause_allocateSoundSlots(int slotCount) asm("func_00225AC0");

int pause_enableSoundOption(void) {
    pause_allocateSoundSlots(1);
    return 0;
}
