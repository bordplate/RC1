#include "common.h"
#include "types.h"
#include "hud.h"

#define HUD_RAM_ALIGN 0x10
#define HUD_RAM_ENTRY_MASK 0x7FFFFFFF
// Word offset of bankLoad (0x74) within HudHeader.
#define HUD_BANKLOAD_WORD_OFFSET 0x1D

extern int hudHeapBase __attribute__((section(".data")));

// Re-bases the HUD palette/texture ram entries of `bank` onto `ram` and marks
// the bank as loaded. A ram entry is an offset within the bank's ram with bit
// 31 used as a flag; the re-base clears the flag and adds the new base.
void LinkHudBank(int bank, char* ram) {
    // The original computes (header + 0x74) + bank * 4; indexing
    // hudHeap.header->bankLoad[bank] directly instead emits
    // header + (bank * 4 + 0x74), which does not match.
    u32* bankLoads = (u32*)hudHeap.header + HUD_BANKLOAD_WORD_OFFSET;
    u32* pLoad = bankLoads + bank;
    // The seed assignment must sit before the guard: EGC keeps it as the
    // bnez delay-slot move, but folds it into the alignment expression
    // when both assignments are in the same basic block.
    char* value = ram;
    if (*pLoad == 0) {
        value = (char*)(((u32)value + HUD_RAM_ALIGN - 1) & ~(u32)(HUD_RAM_ALIGN - 1));
        *pLoad = (u32)value;
        int prev;
        int count;
        if (bank != 0) {
            prev = hudHeap.header->palCount[bank - 1];
        } else {
            prev = 0;
        }
        count = hudHeap.header->palCount[bank];
        for (int i = prev; i < count; i++) {
            hudHeap.pals[i].ram &= HUD_RAM_ENTRY_MASK;
            hudHeap.pals[i].ram += (u32)value;
        }
        if (bank != 0) {
            prev = hudHeap.header->texCount[bank - 1];
        } else {
            prev = 0;
        }
        count = hudHeap.header->texCount[bank];
        for (int i = prev; i < count; i++) {
            hudHeap.texs[i].ram &= HUD_RAM_ENTRY_MASK;
            hudHeap.texs[i].ram += (u32)value;
        }
    }
}

// Unreachable dead tail the original compiler emitted after LinkHudBank
// (0x1FEFC0, matched): a zero store to the bankLoad word
// (sw $zero,0x74($v0)) plus the alignment nop before
// Hud_SendResidentBank. Nothing reaches it (no Ghidra function;
// tools/deadness_scan.py: 0 references), so the bytes are preserved with
// raw asm per the dead-tail policy.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001FF120, 0x4\n"
    "glabel func_001FF120\n"
    "    sw $0, 0x74($2)\n"
    "endlabel func_001FF120\n"
    "    nop\n"
    "    .set reorder\n"
    "    .set at\n"
);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", Hud_SendResidentBank__FiPcb);

void Hud_HeapReset(void) {
    hudHeap.heapEnd = hudHeapBase + 0x64000;
    hudHeap.heapCursor = hudHeapBase;
}

// Bump-allocates a 16-byte-aligned block from the HUD heap. Returns the block
// start (the previous cursor) or 0 when the heap is full; resets the heap
// first if the cursor is still 0.
// EGC collapses the (size + align-1) & ~mask -> new-cursor chain into one
// register, but the original keeps size+align-1 in a1, the aligned size in s0,
// and the new cursor in a0 (updating the old cursor in place). Pin the size,
// the two chain temps, and the cursor to force that allocation.
char* Hud_HeapAlloc(unsigned int size, char* comment, char* file, int line) {
    register unsigned int sz asm("$16");
    sz = size;
    if (hudHeap.heapCursor == 0) {
        Hud_HeapReset();
    }
    if ((int)sz <= (int)(hudHeap.heapEnd - hudHeap.heapCursor)) {
        register u32 s15 asm("$5");
        s15 = sz + (HUD_RAM_ALIGN - 1);
        register u32 aligned asm("$16");
        aligned = s15 & ~(u32)(HUD_RAM_ALIGN - 1);
        register u32 cursor asm("$4");
        cursor = hudHeap.heapCursor;
        char* result = (char*)cursor;
        cursor += aligned;
        hudHeap.heapCursor = cursor;
        return result;
    }
    return 0;
}

// Trailing alignment padding EGC emitted after Hud_HeapAlloc (0x1FF2FC-0x1FF307,
// three nops) before func_001FF308. The C body is 0x74 bytes but the original
// region is 0x80, so emit the padding to keep func_001FF308 at 0x1FF308.
asm("nop");
asm("nop");
asm("nop");

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF308);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF418);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF480);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF500);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF568);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF570);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF5E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF6D8);

extern int hudMessageTimer;

void hud_updateMessageTimer(void) {
    if (hudMessageTimer != 0) {
        hudMessageTimer--;
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF780);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FF958);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", GetIconFrame__Fii);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", GetFrameTex__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FFC30);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_001FFE18);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200078);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200258);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200468);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200600);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200958);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", Hud_sendTexture__FPciiiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200C80);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200E08);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00200F90);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00201110);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00201128);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_00201200);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", func_002012A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/hud", draw_bootImage__Fi);
