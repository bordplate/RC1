#include "common.h"
#include "types.h"
#include "pause.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", SoundOptionsMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", DrawSoundMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021D168);

int pause_soundCallbackNoop(void) {
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021D1F8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021D2C8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021D338);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021D4A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021D948);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021DDF8);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021DF58);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021DF98);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021E110);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021E1F8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021E230);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021E608);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021E698);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021E7C8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", DrawQuitGameMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021EA48);

// Symbol override: this still-assembly helper at 0x00225530 refreshes the
// timestamp field of a pause callback's moby sub-object.
extern int pause_refreshMobyTimestamp(int timestamp) asm("func_00225530");

int pause_updateSelection(int* p) {
    p[0x11] = pause_refreshMobyTimestamp(p[0x11]);
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", DrawItemsMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", DrawGBsShipMenu__maybe);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021F120);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021F158);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021F330);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", DrawMissionsMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021F5F8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", DrawMissionsMenu2);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021F8E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021F990);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021FC68);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021FCE0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021FD78);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021FDC8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", DrawCheckingMemoryCardDataMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00220648);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00220790);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00220850);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00220B20);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00220E28);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", DrawCheatsMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002212B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00221460);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002215F8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002216C0);

int pause_endMenuCallback(void) {
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00221800);

extern int pauseMenuFlags __attribute__((section(".data")));
extern int* pauseCurrentActionList __attribute__((section(".data")));
extern int pauseDefaultActionList __attribute__((section(".data")));

int pause_selectDefaultActionList(void) {
    if (pauseMenuFlags & 0x40) {
        pauseCurrentActionList = &pauseDefaultActionList;
    }
    return 0;
}

// The shared list drawer at 0x001FD748 (still assembly, defined in help.cpp)
// renders two element ranges as a 19-slot list, highlighting the current
// entry. It takes each range as a (start, end) pair and scales the element
// counts by 0x10. Its ELF symbol is the unmangled address placeholder
// func_001FD748, so an asm label -- not C++ mangling -- produces it.
extern void help_drawRangeList(int start1, int end1, int start2, int end2)
    asm("func_001FD748");

// Pause-menu callback invoked through the function-pointer table at 0x1D2264.
// The menu object carries two element ranges: range 1 is base1/count1 and
// range 2 is base2/count2. Each is converted to a (start, end) pair and
// forwarded to the list drawer.
typedef struct {
    u8 pad[0x18];
    u32 base1;
    u32 base2;
    u32 count1;
    u32 count2;
} PauseRangeList;

// Symbol override: the function-pointer table references the unmangled
// address symbol func_00221930, so an asm label -- not an extern "C"
// assumption or C++ mangling -- produces it.
int pause_drawRangeList(PauseRangeList* list) asm("func_00221930");
int pause_drawRangeList(PauseRangeList* list) {
    help_drawRangeList(list->base1, list->base1 + list->count1,
                       list->base2, list->base2 + list->count2);
    return 2;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00221968);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00221A48);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00221AB8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00221B50);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00221D28);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00221D68);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00221E50);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00221F58);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002220F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00222290);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002223F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00222768);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", ObtainAllGoldWeaponsMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", DrawEndScreenMenuMaybe);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00222D98);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00222F18);

typedef struct {
    int pad[18];
    int f48;
} PauseCallback;

int pause_updateCallback(PauseCallback* self) {
    self->f48 = pause_releaseSoundSlot(self->f48);
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", SavingDataMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", LoadingDataMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", SavingDataMenu2);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002239E0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00223E28);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002240C8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002242B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", LoadHandGadget);

void pause_noopA(void) {
}

void pause_noopB(void) {
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00224B70);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00224D28);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00224E18);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00224FC0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002250F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225180);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225490);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225530);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225578);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225660);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002256E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225A68);

void pause_noopC(void) {
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225AC0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225C18);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225CD8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225D88);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225DD8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225E20);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00225E70);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002265D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00226670);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00226718);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00226778);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002267B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00226848);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002269C0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00226A70);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00226B08);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00226E08);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00226E58);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00226F50);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00226FA0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00226FB8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002270E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00227140);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_002271D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00227378);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_00227548);
