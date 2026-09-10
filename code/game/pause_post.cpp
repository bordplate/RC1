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

extern "C" int SetPauseActionList(PauseActionMode* mode) {
    if (*(int *)0x15EE90 != 0)
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

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021CA98);

extern int pauseMemoryCardState __attribute__((section(".data")));

int pause_resetMemoryCardState(void) {
    pauseMemoryCardState = -1;
    return 0;
}

// C linkage: the still-assembly pause sound-slot allocator at 0x00225AC0
// fills the five-entry pause sound table and has an unmangled entry point.
extern "C" void pause_allocateSoundSlots(int param_1);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021DDD0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021DDF8);

int pause_menuCallbackNoop(void) {
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_0021DF30);

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

extern "C" int func_00225530(int);

int pause_updateSelection(int* p) {
    p[0x11] = func_00225530(p[0x11]);
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

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post", func_00221A88);

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

extern "C" int func_00225CD8(int param_1);

typedef struct {
    int pad[18];
    int f48;
} PauseCallback;

int pause_updateCallback(PauseCallback* self) {
    self->f48 = func_00225CD8(self->f48);
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
