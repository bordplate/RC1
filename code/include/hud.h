#ifndef HUD_H
#define HUD_H

#include "types.h"

// Number of HUD channel slots in hudChanSlots.
#define HUD_SLOT_COUNT 13

// Icon id that leaves a channel slot empty/reset: Hud_InitBanks assigns it to
// every slot at startup, and a reset request restores it.
#define HUD_SLOT_RESET_ICON_ID 0xFFFF

// HUD message (hudMsgText1/hudMsgText2) drawing constants used by
// Hud_DrawChannels (func_001FF780).
#define HUD_MSG_DRAW_X 0x100         // x coordinate of the printed message
#define HUD_MSG_IDLE_Y 100           // y coordinate written when no message is fading
#define HUD_MSG_COLOR_RGB 0xF0F0F0   // base RGB (light gray); fade supplies the alpha
#define HUD_MSG_FADE_MAX 0x80        // max fade value; also the per-frame step numerator
#define HUD_MSG_FADE_STEP_FRAMES 8   // frame count scaled by func_001F96F8 for the fade step
#define HUD_MSG_COUNT_RESET 1000     // message-count value at which it is reset to 0
#define HUD_VU_FIELD_INIT 0x00FFFFF0 // value Hud_DrawChannels writes to hudHeap.vuField_0C

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

// One HUD icon-table entry (DL icon_t); the table is terminated by an id of
// 0xFFFF. `len` is the icon's frame count, `start` the index of its first
// frame in the frame table.
typedef struct {
    u16 id;
    u16 len;
    u16 start;
    u8 animType;
    u8 speed;
} HudIconDef;

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
// (0x199B60). Hud_InitBanks zeroes/resets them and Hud_SetChannelPending fills
// the pending block (0x20-0x38) with a channel request. When the request's
// mode & slot->mode & 0x20 holds, Hud_CommitChannel commits the pending block
// into the active block (0x04-0x18), sets up the committed-icon fields
// (0x00/0x40-0x44) via Hud_SetupChannelIcon, and invokes the callback fn.
// b..e are opaque callback parameters (named for the Hud_SetChannelPending
// argument they hold).
typedef struct {
    u32 iconId;      // +0x00: committed icon id (icon-table entry id)
    u32 mode;        // +0x04: committed channel mode (arg & 0xFFF0)
    u32 b;           // +0x08
    u32 c;           // +0x0C
    u32 fn;          // +0x10: channel callback, invoked on commit
    u32 d;           // +0x14
    u32 e;           // +0x18
    u8 pad_1C[4];    // +0x1C
    u32 pendId;      // +0x20
    u32 pendMode;    // +0x24
    u32 pendB;       // +0x28
    u32 pendC;       // +0x2C
    u32 pendFn;      // +0x30
    u32 pendD;       // +0x34
    u32 pendE;       // +0x38
    u8 pad_3C[4];    // +0x3C
    s16 iconIndex;   // +0x40: icon-table index of iconId (Hud_SetupChannelIcon)
    u8 iconAnimType; // +0x42: icon entry's animType
    u8 pad_43;       // +0x43
    u32 iconStart;   // +0x44: icon entry's start (first frame index)
    u8 pad_48[0x1C]; // +0x48
    u32 serial;      // +0x64: assigned slot serial
    u32 pending;     // +0x68: pending flag, cleared on commit
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
    u8 pad_08[4];            // +0x08
    u32 vuField_0C;          // +0x0C: set to HUD_VU_FIELD_INIT by Hud_DrawChannels; the
                               //   upper word of a 64-bit VU1 packet field consumed by
                               //   the per-channel draw callbacks.
    u32 heapCursor;          // +0x10
    u32 heapEnd;             // +0x14
    HudHeader* volatile header; // +0x18
    u32 iconTable;           // +0x1C
    u32 pad_20;              // +0x20
    HudFrameTex* texs;       // +0x24
    HudFramePal* pals;       // +0x28
    u32 pad_2C;              // +0x2C
    u32 field_30;            // +0x30: HUD skip flag; non-zero makes Hud_DrawChannels
                              //   clear it and return early this frame.
} HudHeap;

extern HudHeap hudHeap __attribute__((section(".data")));
extern HudChanSlot hudChanSlots[13];

// Finds the icon-table index of `iconId` (see hud_icon.cpp); the table is
// terminated by an entry with id 0xFFFF.
int Hud_GetIconIndex(int iconId);

// Fills the slot's committed-icon fields (iconId/iconIndex/iconAnimType/
// iconStart) from the icon-table entry for `iconId` (see hud_post_post.cpp).
// Symbol override: the boot ELF is stripped and the Splat linker script pins
// this entry to the address-based placeholder label func_001FF500, which the
// natural cfront mangle (Hud_SetupChannelIcon__FP11HudChanSloti) cannot
// produce.
void Hud_SetupChannelIcon(HudChanSlot* slot, int iconId) asm("func_001FF500");

// Records a pending channel request (see hud_chan.cpp); returns the slot serial.
int Hud_SetChannelPending(int chan, int id, int fn, int d, int e, int c, int b);

// Commits a pending channel request (see hud_post.cpp): sets up the slot icon
// from pendId, copies the pending block into the active block, invokes the
// committed callback fn when it is non-zero, and clears the pending flag.
void Hud_CommitChannel(HudChanSlot*) asm("func_001FF418");

// Finds the slot whose serial matches `serial` and sets its pending mode to
// `mode`; if the slot is not pending, its active mode is set too (see
// hud_post_post.cpp). Passing 0 clears the channel's mode. Symbol override:
// the stripped boot ELF pins this entry to the address-based placeholder
// func_001FF570.
void Hud_SetChannelModeBySerial(int serial, int mode) asm("func_001FF570");

#endif
