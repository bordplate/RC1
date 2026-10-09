#include "common.h"
#include "types.h"
#include "hud.h"

#define HUD_MAIN_BANK_SIZE 0x2800
#define HUD_AUX_BANK_SIZE 0x1400

void* Hud_HeapAlloc(unsigned int, char*, char*, int);
void FastMemZero16(void*, int);

// HUD bank RAM words (0x15FA00-0C), all in .lit (already generated data).
// hudBankMain/hudBankEnd/hudBankAux stay self-based absolute; hudBankBaseGp
// must be a GPREL16 store, so it uses the .extern-seeded alias in
// config/linker_aliases.ld that ps2eeas expands as GPREL16.
extern void* hudBankMain;
extern void* hudBankEnd;
extern void* hudBankAux;
extern void* hudBankBaseGp;
extern char hudFileStr[];
// A plain declaration expands self-based under ps2eeas; seed hudBankBaseGp so
// its store is GPREL16, matching the original.
asm(".extern hudBankBaseGp, 4");

// Reset the HUD channel slots and allocate/zero the main + aux HUD banks.
// Called from Transition_DoTransition immediately before LoadHudBanks.
void Hud_InitBanks(void) {
    hudHeap.nextSlotSerial = 0;
    hudHeap.field_04 = 0;

    // EGC folds a constant pointer offset into the table symbol; pin the base
    // to v1 so it stays a separate lui/addiu, matching the original's
    // `lui v1, table; addiu v1, lo; addiu s0, v1, 0x24` anchor setup. The loop
    // then walks the slots from a running anchor at slot+0x24 using the
    // original's anchor-relative word offsets (a codegen artifact, not the
    // struct layout).
    register int* tableBase asm("$3");
    tableBase = (int*)hudChanSlots;
    int* anchor = tableBase + 9;
    int i = 0;
    do {
        anchor[0x10] = -1;
        anchor[-1] = 0x10000;
        Hud_SetChannelPending(i, 0xFFFF, 0, 0, 0, 0, 1);
        i++;
        anchor[0x16] = 0;
        anchor[0x12] = -6;
        anchor[-8] = 0;
        anchor[0] = 0;
        anchor += 0x24;
    } while (i < HUD_SLOT_COUNT);

    if (hudBankMain == 0) {
        // 0x115/0x116 are the original __LINE__ values (277/278) passed for the
        // allocation error message; they cannot be regenerated.
        hudBankMain = Hud_HeapAlloc(HUD_MAIN_BANK_SIZE, 0, hudFileStr, 0x115);
        hudBankAux = Hud_HeapAlloc(HUD_AUX_BANK_SIZE, 0, hudFileStr, 0x116);
    }
    hudBankEnd = (void*)((u8*)hudBankMain + HUD_MAIN_BANK_SIZE);
    hudBankBaseGp = hudBankMain;
    FastMemZero16(hudBankMain, HUD_MAIN_BANK_SIZE);
    ((u8*)hudBankMain)[0x20] = 0xFF;
}
