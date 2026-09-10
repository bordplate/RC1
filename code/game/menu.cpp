#include "common.h"
#include "types.h"

extern "C" int menu_pointIsClockwise(int a, int b, int c, int d, int e, int f);

extern "C" int func_00206978(int x, int y) {
    return menu_pointIsClockwise(x, y, 0xD3, 0xDB, 0x129, 0xF9);
}

extern u8 D_0013D394 __attribute__((section(".data")));

extern "C" int func_002069A0(void) {
    return D_0013D394 != 0;
}

extern u8 D_0013D395 __attribute__((section(".data")));

extern "C" int func_002069B0(void) {
    return D_0013D395 != 0;
}

extern u8 D_0013D39D __attribute__((section(".data")));

extern "C" int func_002069C0(void) {
    return D_0013D39D != 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_002069D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00206A90);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00206B10);

extern "C" unsigned int D_0015FD64;

extern "C" int func_00206B78(void) {
    return D_0015FD64 < 1;
}

extern u8 D_0013D3A4 __attribute__((section(".data")));

extern "C" int func_00206B88(void) {
    return D_0013D3A4 != 0;
}

extern u8 D_0013D3A5 __attribute__((section(".data")));

extern "C" int func_00206B98(void) {
    return D_0013D3A5 != 0;
}

extern u8 D_0013D3A6 __attribute__((section(".data")));

extern "C" int func_00206BA8(void) {
    return D_0013D3A6 != 0;
}

extern u8 D_0013D3A7 __attribute__((section(".data")));

extern "C" int func_00206BB8(void) {
    return D_0013D3A7 != 0;
}

extern u8 D_0013D3AD __attribute__((section(".data")));

extern "C" int func_00206BC8(void) {
    return D_0013D3AD != 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00206BD8);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00206E18);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00206F50);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207100);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_002071C0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207250);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207300);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_002073B8);

// D_0013D3BD: menu item enabled-flag byte in the 0x13D394 per-item family;
// the specific item is unconfirmed.
extern u8 D_0013D3BD __attribute__((section(".data")));
// D_001413DC: menu state word; func_00207480 checks it for == 0xF. Unconfirmed.
extern u32 D_001413DC __attribute__((section(".data")));

// func_00207480: menu predicate callback (jump table vram 0x19FF70, entry
// 0x19FF90). Returns the D_0013D3BD flag for y < 0x101, else D_001413DC == 0xF.
// C linkage: emitted as the unmangled symbol the menu jump table references.
// The specific menu item/state this predicate gates is unconfirmed, so the
// address-based name is retained.
extern "C" int func_00207480(int x, int y) {
    // Negated condition is match-critical: EGC must keep the 2-instruction
    // body (lbu; sltu) in the fall-through with the beqz target on the
    // 3-instruction else; (y < 0x101) emits bnez and swaps the two blocks.
    if (y >= 0x101)
        return (D_001413DC ^ 0xF) < 1;
    return D_0013D3BD != 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_002074B0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207508);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207580);

// func_002075F8: menu predicate callback (jump table vram 0x19FF70, entry
// 0x19FFA0). Returns
// 1 only when x < 224 and y <= 38.0f; the gated menu item is unconfirmed, so
// the address-based name is retained. C linkage: emitted as the unmangled
// symbol the menu jump table references.
// Match-sensitive: the used float is the 3rd float param — EGC packs float
// args into 64-bit pairs (f12:f13, f14:f15), so the 3rd lands in $f14 as the
// original expects. The asm nop is the mtc1 -> c.le.s FPU hazard.
extern "C" int func_002075F8(int x, float unused1, float unused2, float y) {
    if (x >= 224)
        return 0;
    float threshold = 38.0f;
    asm volatile("nop" : : "f"(threshold));
    return y <= threshold;
}

extern u8 D_0013D3B8 __attribute__((section(".data")));

extern "C" int func_00207630(void) {
    return D_0013D3B8 != 0;
}

extern u8 D_0013D3B9 __attribute__((section(".data")));

extern "C" int func_00207640(void) {
    return D_0013D3B9 != 0;
}

extern u8 D_0013D3BA __attribute__((section(".data")));

extern "C" int func_00207650(void) {
    return D_0013D3BA != 0;
}

extern u8 D_0013D3CB __attribute__((section(".data")));

extern "C" int func_00207660(void) {
    return D_0013D3CB != 0;
}

extern u8 D_0013D3CC __attribute__((section(".data")));

extern "C" int func_00207670(void) {
    return D_0013D3CC != 0;
}

extern u8 D_0013D3CD __attribute__((section(".data")));

extern "C" int func_00207680(void) {
    return D_0013D3CD != 0;
}

extern u8 D_0013D3D8 __attribute__((section(".data")));

extern "C" int func_00207690(int x, float unused1, float unused2, float y) {
    if (x >= 190)
        return D_0013D3D8 != 0;
    float threshold = 58.5f;
    asm volatile("nop" : : "f"(threshold));
    return threshold <= y;
}

extern u8 D_0013D3D4 __attribute__((section(".data")));

extern "C" int func_002076D0(void) {
    return D_0013D3D4 != 0;
}

extern u8 D_0013D3D5 __attribute__((section(".data")));

extern "C" int func_002076E0(void) {
    return D_0013D3D5 != 0;
}

extern u8 D_0013D3D6 __attribute__((section(".data")));

extern "C" int func_002076F0(void) {
    return D_0013D3D6 != 0;
}

extern u8 D_0013D3D7 __attribute__((section(".data")));

extern "C" int func_00207700(void) {
    return D_0013D3D7 != 0;
}

extern u8 D_0013D3D9 __attribute__((section(".data")));

extern "C" int func_00207710(void) {
    return D_0013D3D9 != 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207720);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_002077A0);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207800);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207880);

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207930);

extern u8 D_0013D3E1 __attribute__((section(".data")));

extern "C" int func_002079C8(void) {
    return D_0013D3E1 != 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_002079D8);

extern "C" int func_00207A08(void) {
    return 1;
}

extern "C" int func_00207A10(void) {
    return 1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/menu", func_00207A18);

extern u8 D_0013D3FA __attribute__((section(".data")));

extern "C" int func_00207AB8(void) {
    return D_0013D3FA != 0;
}

extern u8 D_0013D3FB __attribute__((section(".data")));

extern "C" int func_00207AC8(void) {
    return D_0013D3FB != 0;
}

extern u8 D_0013D3FC __attribute__((section(".data")));

extern "C" int func_00207AD8(void) {
    return D_0013D3FC != 0;
}

extern u8 D_0013D3FD __attribute__((section(".data")));

extern "C" int func_00207AE8(void) {
    return D_0013D3FD != 0;
}

extern u8 D_0013D407 __attribute__((section(".data")));

extern "C" int func_00207AF8(void) {
    return D_0013D407 != 0;
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

extern "C" void func_00208810(void) {
}
