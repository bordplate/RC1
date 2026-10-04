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
    PauseSpriteEntry* first = (PauseSpriteEntry*)mode->spriteLists[1];
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

// C++ linkage (mangles to FastAddRots__Fff): fast add-with-wrap in
// game/fastfunc; returns a + b wrapped into [-100.0, 100.0).
extern float FastAddRots(float a, float b);

// Pause-screen step object allocated by func_00225490: camera-relative
// display floats at 0x10-0x18, a wrapping value at 0x40 (stepped 0.02 by the
// 0x21F120 callback) and the wrapping angle at 0x48 (stepped 0.01 by the
// 0x21E1F8 callback); the per-frame callback is stored at 0x74.
typedef struct {
    u8 pad[0x40];
    float step02;
    u8 pad2[4];
    float angle;
} PauseStepObject;

// Symbol override: per-frame pause step callback at the address-based
// generated entry point; advances the object's angle by 0.01 via FastAddRots.
void pause_stepAngle01(PauseStepObject* obj) asm("func_0021E1F8");

void pause_stepAngle01(PauseStepObject* obj) {
    obj->angle = FastAddRots(obj->angle, 0.01f);
}

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

// Symbol override: per-frame pause step callback at the address-based
// generated entry point; advances the object's 0x40 value by 0.02 via
// FastAddRots.
void pause_stepAngle02(PauseStepObject* obj) asm("func_0021F120");

void pause_stepAngle02(PauseStepObject* obj) {
    obj->step02 = FastAddRots(obj->step02, 0.02f);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post2", func_0021F158);

// C++ linkage (mangles to VU1_addGSregister__FUiUlb): appends a GS register
// value record to the VU1 chain. Declared 2-param; the extra trailing params
// in the mangled name are never materialized at any RC1 call site.
void VU1_addGSregister(unsigned int reg, unsigned long value)
    asm("VU1_addGSregister__FUiUlb");

// No-map-available screen setup (draw_post_post 0x205640); no args, void.
// Unmangled symbol per config/symbols.txt.
extern void UNK_NoMapAvailable(void) asm("UNK_NoMapAvailable");

// Symbol override: no-map-available screen setup at the address-based
// generated entry point; stages two GS register records then shows the
// no-map screen, returning 8.
int pause_noMapSetup(void) asm("func_0021F330");

int pause_noMapSetup(void) {
    VU1_addGSregister(0x42, 0x44);
    VU1_addGSregister(0x47, 0xb);
    UNK_NoMapAvailable();
    return 8;
}

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

