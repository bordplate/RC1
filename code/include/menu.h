#ifndef RC1_MENU_H
#define RC1_MENU_H

// The async memory-card I/O state machine (g_memcard in the Deadlocked
// reference). The boot save/load "menu" is a front end for it: the page
// callbacks below set nextstage/nextslot to request an operation and wait
// for stage to return to idle. memcard_Update (0x2093D8) is the 24-stage
// machine driven through the control block; its probe cycle addresses card
// records from the struct base at 0xB8 stride (slot * 0xB8), of which only
// record 0 sits inside the struct (the control block follows at +0xB8).
// Field names follow the reference; see
// notes/menu_post_mid_menu_post_pageAdvance.md.
struct memcard_data {
    int port;        // +0x00 passed as a scalar to the card probe
    int slot;        // +0x04 passed as a scalar to the card probe
    int type;        // +0x08 probe result
    int free;        // +0x0C probe result
    char pad_10[0x4];
    int format;      // +0x14 -3 = unformatted sentinel; holds the requested
                    // save size while a save is pending
    char pad_18[0x4];
    int changed;     // +0x1C set to res when the card content changes; the
                    // boot menu reads record 0's word as the
                    // pending-selection flag (aliased by
                    // menu_postHasPendingSelection)
    char info[0x8C]; // +0x20 five 28-byte save-slot info records
    int oldsave;     // +0xAC
    char pad_B0[0x8];
};                     // record size 0xB8

struct memcard_globals {
    memcard_data card; // +0x00
    int cmd;           // +0xB8 first word of the embedded sceMcSync buffer
    int res;           // +0xBC second sync-buffer word; nonzero marks a card
                      // content change
    int useslot;       // +0xC0
    int slot;          // +0xC4 card index the machine operates on
    int field_0xC8;    // +0xC8 initial value 1; positional match of the
                       // reference's level (initial 0 there), usage
                       // unestablished
    int sync;          // +0xCC nonzero while an async card operation is
                       // active; memcard_Update polls sceMcSync and returns
                       // without running a stage
    int file;          // +0xD0
    int stage;         // +0xD4
    int substage;      // +0xD8
    int nextstage;     // +0xDC
    int nextslot;      // +0xE0
    int err;           // +0xE4
    int errslot;       // +0xE8
    char* addr;        // +0xEC save/load buffer
    int size;          // +0xF0
    int autosave;      // +0xF4 one-shot save request flag
    int checkslot;     // +0xF8
    int timer;         // +0xFC
    int savesize;      // +0x100
};

extern memcard_globals g_memcard __attribute__((section(".data")));
extern int menuPostCallbackIndex __attribute__((section(".data")));
extern int menuPostFlags __attribute__((section(".data")));
// g_memcard.card.changed viewed as a standalone global: 0x13D2AC is
// 0x13D290 + 0x1C. Both spellings are required in source; the direct
// symbol loads and the struct-field loads compile to different register
// forms that the original uses in different functions.
extern int menu_postHasPendingSelection __attribute__((section(".data")));

#define MENU_POST_SUBMENU_ENABLED_FLAG 0x1
// menuPostFlags bits whose meaning is not yet established.
#define MENU_POST_FLAG_02 0x2
#define MENU_POST_FLAG_04 0x4
#define MENU_POST_PAGE_OPEN_BLOCKED_FLAG 0x40
// Sentinel compared against g_memcard.card.changed by the post callbacks.
#define MENU_POST_SELECTION_NONE -2
#define MENU_POST_SELECT_NEXT_PAGE_CALLBACK 3
#define MENU_POST_PROCESS_PAGE_SELECTION_CALLBACK 4
// nextstage request: format the card if unformatted (memcard stage 3).
#define MEMCARD_REQUEST_FORMAT 3
#define MENU_POST_OPEN_PAGE_CALLBACK 8
// Post-callback table slots whose handlers are still INCLUDE_ASM.
#define MENU_POST_FLAG_RESOLVE_CALLBACK 6
#define MENU_POST_PAGE_ADVANCE_CALLBACK 10
#define MENU_POST_BOOT_FLAG_RESOLVE_CALLBACK 13
#define MENU_POST_UNSET_PAGE_INDEX -1
// stage value of the memcard machine: 2 is idle (it will pick up a pending
// nextstage/nextslot request on its next tick). The page-open callback
// (slot 8) and the page-advance callback (slot 10) test for it before they
// act; bmain spins memcard_Update while stage >= 3 (an operation is running)
// or nextstage >= 0 (a request is pending). Stages 0/1 are the probe/record
// cycle and 3..23 are the active operations (see the jtbl dispatch in
// memcard_Update).
#define MEMCARD_STAGE_IDLE 2
// nextstage request: scan/getdir the card (memcard stage 7). Written by the
// page-advance callback (slot 10).
#define MEMCARD_REQUEST_SCAN 7
// Slot 11 of the post-callback table, queued by the page-advance callback
// (slot 10) and the page-open callback (slot 8). After a page opens or
// advances it routes the chain to the next step based on the just-read
// memcard slot. The handler (0x208C00) is still INCLUDE_ASM.
#define MENU_POST_PAGE_OPEN_DECISION_CALLBACK 11

#endif
