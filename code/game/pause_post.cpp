#include "common.h"
#include "types.h"

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

// C linkage: this callback is referenced by the original unmangled pause menu
// action-list table.
extern "C" int SetPauseActionList(PauseActionMode* mode) {
    if (*(int*)0x15EE90 != 0)
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

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021CA60);

// Pause menu callback: select one of two sprite-index lists based on a
// dedicated flag and store the chosen list pointer in the menu item's field
// at offset 0x34 (element 1 of the 7-entry pointer array that begins at
// offset 0x30, which func_0021CA60 copies to D_00141EA0).
typedef struct {
    u8 pad[0x34];
    u32 spriteList;
} PauseSpriteListMode;

extern u8 pauseSpriteListFlag __attribute__((section(".data")));
extern int pauseSpriteListA[];
extern int pauseSpriteListB[];

int pause_selectSpriteList(PauseSpriteListMode* mode) {
    // Match-critical: the != 0 direction makes EGC emit the original beqzl
    // layout; the == 0 form emits bnez (a 3-byte branch-direction diff).
    mode->spriteList =
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

extern int pauseSoundVolume __attribute__((section(".data")));

int pause_updateSoundVolume(void) {
    pauseSoundVolume = *(int*)0x15EDF0 * 8 / 10;
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", SoundOptionsMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", DrawSoundMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021D168);

int pause_soundCallbackNoop(void) {
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021D1F8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021D2C8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021D338);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021D4A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021D948);

// One 0xC-byte record as the pause sprite lists are walked (the consumers
// stride the packed lists by 0xC): the sprite id at +0x00 plus the per-entry
// action tag at +0x02 consumed by the 0x21ABF8 select handler and the
// 0x21B1C8 drawer (tag 0 draws the entry dimmed, no action).
typedef struct {
    u16 spriteId;
    u16 actionTag;
    u8 pad_04[8];
} PauseSpriteEntry;

// Action tags this callback can select, keyed on pauseSpriteTagByte: 0 draws
// the entry dimmed with no action; 3 makes the select handler set the
// next-sprite state from the entry's +0x04 field.
#define PAUSE_SPRITE_ACTION_NONE 0
#define PAUSE_SPRITE_ACTION_NEXT 3

// Pause-menu item callback: sets the first sprite-list entry's action tag to
// PAUSE_SPRITE_ACTION_NEXT, or to PAUSE_SPRITE_ACTION_NONE while the shared
// state byte equals 1.
extern u8 pauseSpriteTagByte __attribute__((section(".data")));

int pause_setFirstSpriteTag(PauseSpriteListMode* mode) {
    PauseSpriteEntry* first = (PauseSpriteEntry*)mode->spriteList;
    first->actionTag = (pauseSpriteTagByte == 1) ? PAUSE_SPRITE_ACTION_NONE
                                                 : PAUSE_SPRITE_ACTION_NEXT;
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021DDF8);

int pause_menuCallbackNoop(void) {
    return 0;
}

typedef struct {
    u8 pad[0x34];
    u32 field_34;
    float field_38;
    u8 pad_3c[8];
    u32 field_44;
    u32 field_48;
} PauseMenuEntry;

// Symbol override: pause-menu callback at the address-based generated entry
// point, referenced by the original function-pointer tables. Resets an entry:
// clears field_34/44/48 and sets the field_38 angle to PI.
int pause_resetMenuEntry(PauseMenuEntry* entry) asm("func_0021DF30");

int pause_resetMenuEntry(PauseMenuEntry* entry) {
    entry->field_34 = 0;
    entry->field_38 = 3.14159265f;
    entry->field_44 = 0;
    entry->field_48 = 0;
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021DF58);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021DF98);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021E110);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021E1F8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021E230);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021E608);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021E698);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021E7C8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", DrawQuitGameMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021EA48);

// Symbol override: this still-assembly helper at 0x00225530 refreshes the
// timestamp field of a pause callback's moby sub-object.
extern int pause_refreshMobyTimestamp(int timestamp) asm("func_00225530");

int pause_updateSelection(int* p) {
    p[0x11] = pause_refreshMobyTimestamp(p[0x11]);
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", DrawItemsMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", DrawGBsShipMenu__maybe);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021F120);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021F158);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021F330);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", DrawMissionsMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021F5F8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", DrawMissionsMenu2);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021F8E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021F990);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021FC68);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021FCE0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021FD78);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021FDC8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", DrawCheckingMemoryCardDataMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00220648);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00220790);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00220850);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00220B20);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00220E28);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", DrawCheatsMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002212B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00221460);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002215F8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002216C0);

int pause_endMenuCallback(void) {
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00221800);

extern int pauseMenuFlags __attribute__((section(".data")));
extern int* pauseCurrentActionList __attribute__((section(".data")));
extern int pauseDefaultActionList __attribute__((section(".data")));

int pause_selectDefaultActionList(void) {
    if (pauseMenuFlags & 0x40) {
        pauseCurrentActionList = &pauseDefaultActionList;
    }
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00221930);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00221968);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00221A48);

// Symbol override: this still-assembly helper at 0x00225CD8 releases a pause
// sound slot and stops its active music state when necessary.
extern int pause_releaseSoundSlot(int slot) asm("func_00225CD8");

typedef struct {
    u8 pad[0x54];
    u32 f54;
} PauseSoundSlotState;

int pause_releaseStateSoundSlot(PauseSoundSlotState* state) {
    state->f54 = pause_releaseSoundSlot(state->f54);
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00221AB8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00221B50);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00221D28);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00221D68);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00221E50);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00221F58);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002220F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00222290);

typedef struct {
    u8 pad[0x3C];
    u32 field_3c;
    u32 field_40;
    u8 pad_44[0xC];
    u32 field_50;
} PauseMenuState;

int pause_resetMenuState(PauseMenuState* menu) {
    menu->field_40 = 0;
    menu->field_50 = 0;
    menu->field_3c = 0;
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002223F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00222768);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", ObtainAllGoldWeaponsMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", DrawEndScreenMenuMaybe);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00222D98);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00222F18);

typedef struct {
    int pad[18];
    int f48;
} PauseCallback;

int pause_updateCallback(PauseCallback* self) {
    self->f48 = pause_releaseSoundSlot(self->f48);
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", SavingDataMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", LoadingDataMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", SavingDataMenu2);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002239E0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00223E28);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002240C8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002242B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", LoadHandGadget);

void pause_noopA(void) {
}

void pause_noopB(void) {
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00224B70);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00224D28);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00224E18);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00224FC0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002250F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225180);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225490);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225530);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225578);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225660);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002256E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225A68);

void pause_noopC(void) {
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225AC0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225C18);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225CD8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225D88);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225DD8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225E20);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00225E70);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002265D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00226670);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00226718);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00226778);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002267B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00226848);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002269C0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00226A70);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00226B08);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00226E08);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00226E58);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00226F50);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00226FA0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00226FB8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002270E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00227140);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_002271D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00227378);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00227548);
