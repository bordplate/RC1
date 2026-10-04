#include "common.h"
#include "types.h"
#include "pause.h"

// Symbol override: this still-assembly helper at 0x00225CD8 releases a pause
// sound slot and stops its active music state when necessary.
extern int pause_releaseSoundSlot(int slot) asm("func_00225CD8");

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00221D68);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00221E50);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00221F58);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_002220F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00222290);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_002223F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00222768);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", ObtainAllGoldWeaponsMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", DrawEndScreenMenuMaybe);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00222D98);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00222F18);

typedef struct {
    int pad[18];
    int f48;
} PauseCallback;

int pause_updateCallback(PauseCallback* self) {
    self->f48 = pause_releaseSoundSlot(self->f48);
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", SavingDataMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", LoadingDataMenu);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", SavingDataMenu2);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_002239E0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00223E28);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_002240C8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_002242B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", LoadHandGadget);

void pause_noopA(void) {
}

void pause_noopB(void) {
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00224B70);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00224D28);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00224E18);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00224FC0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_002250F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00225180);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00225490);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00225530);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00225578);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00225660);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_002256E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00225A68);

void pause_noopC(void) {
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00225AC0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00225C18);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00225CD8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00225D88);

// Symbol override: lookup at the address-based generated entry point; scans
// five key/value pairs (key at pauseLookupKeys, value at key+1) for the
// target, ORs bit 2 (0x4) into the matching value, returns 0 on match else 1.
// The base variable forces the two-step pointer setup (base, then base+4)
// that EGC otherwise folds into a single constant-offset load.
extern int pauseLookupKeys[];
int pause_lookupSetFlag(int target) asm("func_00225DD8");

int pause_lookupSetFlag(int target) {
    int* base = pauseLookupKeys;
    int* p = base + 1;
    int i = 0;
    do {
        i++;
        if (p[-1] != target) {
            p += 2;
            continue;
        }
        *p |= 0x4;
        return 0;
    } while (i < 5);
    return 1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00225E20);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00225E70);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_002265D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00226670);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00226718);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00226778);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_002267B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00226848);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_002269C0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00226A70);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00226B08);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00226E08);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00226E58);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00226F50);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00226FA0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00226FB8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_002270E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00227140);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_002271D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00227378);

INCLUDE_ASM("code/_generated/nonmatchings/game/pause_post3", func_00227548);
