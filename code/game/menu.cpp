#include "common.h"
#include "types.h"

// C linkage: this callback is installed through the original unmangled menu
// jump table entry.
extern "C" int menu_pointIsClockwise(int a, int b, int c, int d, int e, int f);

int menu_isClockwiseForItem(int x, int y) {
    return menu_pointIsClockwise(x, y, 0xD3, 0xDB, 0x129, 0xF9);
}

extern u8 menuItemEnabled_0 __attribute__((section(".data")));

int menu_isItemEnabled_0(void) {
    return menuItemEnabled_0 != 0;
}

extern u8 menuItemEnabled_1 __attribute__((section(".data")));

int menu_isItemEnabled_1(void) {
    return menuItemEnabled_1 != 0;
}

extern u8 menuItemEnabled_9 __attribute__((section(".data")));

int menu_isItemEnabled_9(void) {
    return menuItemEnabled_9 != 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_002069D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00206A90);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00206B10);

extern unsigned int menuSelectionCount;

int menu_isSelectionCountZero(void) {
    return menuSelectionCount < 1;
}

extern u8 menuItemEnabled_16 __attribute__((section(".data")));

int menu_isItemEnabled_16(void) {
    return menuItemEnabled_16 != 0;
}

extern u8 menuItemEnabled_17 __attribute__((section(".data")));

int menu_isItemEnabled_17(void) {
    return menuItemEnabled_17 != 0;
}

extern u8 menuItemEnabled_18 __attribute__((section(".data")));

int menu_isItemEnabled_18(void) {
    return menuItemEnabled_18 != 0;
}

extern u8 menuItemEnabled_19 __attribute__((section(".data")));

int menu_isItemEnabled_19(void) {
    return menuItemEnabled_19 != 0;
}

extern u8 menuItemEnabled_25 __attribute__((section(".data")));

int menu_isItemEnabled_25(void) {
    return menuItemEnabled_25 != 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00206BD8);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00206E18);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00206F50);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207100);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_002071C0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207250);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207300);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_002073B8);

extern u8 menuItemEnabled_41 __attribute__((section(".data")));
// Menu state word used by the map availability predicate.
extern u32 menuSelectionState __attribute__((section(".data")));

// Menu predicate callback at jump table entry 0x19FF90.
int menu_isMapItemAvailable(int x, int y) {
    // Negated condition is match-critical: EGC must keep the 2-instruction
    // body (lbu; sltu) in the fall-through with the beqz target on the
    // 3-instruction else; (y < 0x101) emits bnez and swaps the two blocks.
    if (y >= 0x101)
        return (menuSelectionState ^ 0xF) < 1;
    return menuItemEnabled_41 != 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_002074B0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207508);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207580);

// Menu predicate callback at jump table entry 0x19FFA0.
// Match-sensitive: the used float is the 3rd float param — EGC packs float
// args into 64-bit pairs (f12:f13, f14:f15), so the 3rd lands in $f14 as the
// original expects. The asm nop is the mtc1 -> c.le.s FPU hazard.
int menu_isItemAvailableBelowHeight(int x, float unused1, float unused2, float y) {
    if (x >= 224)
        return 0;
    float threshold = 38.0f;
    asm volatile("nop" : : "f"(threshold));
    return y <= threshold;
}

extern u8 menuItemEnabled_36 __attribute__((section(".data")));

int menu_isItemEnabled_36(void) {
    return menuItemEnabled_36 != 0;
}

extern u8 menuItemEnabled_37 __attribute__((section(".data")));

int menu_isItemEnabled_37(void) {
    return menuItemEnabled_37 != 0;
}

extern u8 menuItemEnabled_38 __attribute__((section(".data")));

int menu_isItemEnabled_38(void) {
    return menuItemEnabled_38 != 0;
}

extern u8 menuItemEnabled_55 __attribute__((section(".data")));

int menu_isItemEnabled_55(void) {
    return menuItemEnabled_55 != 0;
}

extern u8 menuItemEnabled_56 __attribute__((section(".data")));

int menu_isItemEnabled_56(void) {
    return menuItemEnabled_56 != 0;
}

extern u8 menuItemEnabled_57 __attribute__((section(".data")));

int menu_isItemEnabled_57(void) {
    return menuItemEnabled_57 != 0;
}

extern u8 menuItemEnabled_68 __attribute__((section(".data")));

int menu_isItemAvailableAboveHeight(int x, float unused1, float unused2, float y) {
    if (x >= 190)
        return menuItemEnabled_68 != 0;
    float threshold = 58.5f;
    asm volatile("nop" : : "f"(threshold));
    return threshold <= y;
}

extern u8 menuItemEnabled_64 __attribute__((section(".data")));

int menu_isItemEnabled_64(void) {
    return menuItemEnabled_64 != 0;
}

extern u8 menuItemEnabled_65 __attribute__((section(".data")));

int menu_isItemEnabled_65(void) {
    return menuItemEnabled_65 != 0;
}

extern u8 menuItemEnabled_66 __attribute__((section(".data")));

int menu_isItemEnabled_66(void) {
    return menuItemEnabled_66 != 0;
}

extern u8 menuItemEnabled_67 __attribute__((section(".data")));

int menu_isItemEnabled_67(void) {
    return menuItemEnabled_67 != 0;
}

extern u8 menuItemEnabled_69 __attribute__((section(".data")));

int menu_isItemEnabled_69(void) {
    return menuItemEnabled_69 != 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207720);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_002077A0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207800);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207880);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207930);

extern u8 menuItemEnabled_77 __attribute__((section(".data")));

int menu_isItemEnabled_77(void) {
    return menuItemEnabled_77 != 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_002079D8);

int menu_alwaysAvailable_1(void) {
    return 1;
}

int menu_alwaysAvailable_2(void) {
    return 1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207A18);

extern u8 menuItemEnabled_102 __attribute__((section(".data")));

int menu_isItemEnabled_102(void) {
    return menuItemEnabled_102 != 0;
}

extern u8 menuItemEnabled_103 __attribute__((section(".data")));

int menu_isItemEnabled_103(void) {
    return menuItemEnabled_103 != 0;
}

extern u8 menuItemEnabled_104 __attribute__((section(".data")));

int menu_isItemEnabled_104(void) {
    return menuItemEnabled_104 != 0;
}

extern u8 menuItemEnabled_105 __attribute__((section(".data")));

int menu_isItemEnabled_105(void) {
    return menuItemEnabled_105 != 0;
}

extern u8 menuItemEnabled_115 __attribute__((section(".data")));

int menu_isItemEnabled_115(void) {
    return menuItemEnabled_115 != 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207B08);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207BB0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207C28);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207E58);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00208028);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00208030);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00208280);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00208408);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00208500);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00208508);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00208770);

void menu_noop(void) {
}
