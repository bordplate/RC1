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

// 0x90-byte HUD channel-slot record; 13 of these sit at hudChanSlots
// (0x199B60). Hud_InitBanks zeroes/resets them and the slot allocator
// (func_001FF308) fills the rest. Most field meanings are not established yet.
typedef struct {
    u32 field_00;    // +0x00
    u32 modeBits;    // +0x04: mode flags ANDed against the slot mode
    u8 pad_08[0x18]; // +0x08
    u32 field_20;    // +0x20
    u32 mode;        // +0x24: slot mode (arg & 0xFFF0)
    u32 field_28;    // +0x28
    u32 field_2C;    // +0x2C
    u32 field_30;    // +0x30
    u32 field_34;    // +0x34
    u32 field_38;    // +0x38
    u8 pad_3C[0x28]; // +0x3C
    u32 serial;      // +0x64: assigned slot serial
    u32 field_68;    // +0x68
    u32 field_6C;    // +0x6C
    u32 field_70;    // +0x70
    u8 pad_74[0x8];  // +0x74
    u32 field_7C;    // +0x7C
    u32 field_80;    // +0x80
    u8 pad_84[0xC];  // +0x84
} HudChanSlot; // 0x90

// HUD heap state (0x19A3E8). header is volatile: SetupGifPaging's loops
// re-read it every iteration. texs/pals are plain: EGC hoists the per-iteration
// field load out of the loop into a pre-loop copy plus a reload in the
// back-branch -likely delay slot (which the R5900 executes on the taken
// edge); a volatile field instead keeps the load at the loop head with a
// plain back-branch, which does not match.
typedef struct {
    u32 nextSlotSerial;      // +0x00: next slot serial; cleared by Hud_InitBanks,
                             //   incremented and assigned by the slot allocator.
    u32 field_04;            // +0x04: cleared by Hud_InitBanks (meaning not established).
    u8 pad_08[0x08];         // +0x08
    u32 heapCursor;          // +0x10
    u32 heapEnd;             // +0x14
    HudHeader* volatile header; // +0x18
    u32 iconTable;           // +0x1C
    u32 pad_20;              // +0x20
    HudFrameTex* texs;       // +0x24
    HudFramePal* pals;       // +0x28
    u32 pad_2C;              // +0x2C
} HudHeap;

extern HudHeap hudHeap __attribute__((section(".data")));
extern HudChanSlot hudChanSlots[13];

#endif
