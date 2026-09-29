#ifndef HUD_H
#define HUD_H

#include "types.h"

// Loaded HUD bank header. Offsets pal_count[4] (0x24) and tex_count[4] (0x44)
// are confirmed by SetupGifPaging's per-iteration count reloads, and
// bank_load[0] (0x74) by LoadCompressedHudBank's clear store; the remaining
// fields follow Deadlocked's hud_header_t.
typedef struct {
    s16 iconCount;
    s16 frameCount;
    u32 iconListOffset;
    u32 frameListOffset;
    u32 palListOffset;
    u32 texListOffset;
    int palCount[8];
    int texCount[8];
    u32 bankSize[8];
    u32 bankLoad[8];
    int bankHandle[8];
} HudHeader;

// One HUD texture slot. DL frameTex_t; the gsram halfword holds the
// texture's GS RAM address in 1/256 units.
typedef struct {
    u32 ram;
    u16 gsram;
    u8 pad_06[2];
} HudFrameTex;

// One HUD palette slot. DL framePal_t; gsram like HudFrameTex.gsram.
typedef struct {
    u32 ram;
    u16 gsram;
    u16 pad_06;
} HudFramePal;

// HUD heap state (0x19A3E8). header is volatile: SetupGifPaging's loops
// re-read it every iteration. texs/pals are plain: EGC hoists the per-iteration
// field load out of the loop into a pre-loop copy plus a reload in the
// back-branch -likely delay slot (which the R5900 executes on the taken
// edge); a volatile field instead keeps the load at the loop head with a
// plain back-branch, which does not match.
typedef struct {
    u8 pad[0x10];
    u32 heapCursor;
    u32 heapEnd;
    HudHeader* volatile header;
    u32 iconTable;
    u32 pad_20;
    HudFrameTex* texs;
    HudFramePal* pals;
    u32 pad_2C;
} HudHeap;

extern HudHeap hudHeap __attribute__((section(".data")));

#endif
