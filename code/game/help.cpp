#include "common.h"
#include "levelmem.h"
#include "camera.h"
#include "font.h"

// Current VU1 chain buffer base in main memory, set by vuChain_getCurrent.
extern int currentVuChain;

// Allocates and starts a sound channel (sound.cpp, C linkage).
extern "C" void func_0022DB10(int a, int b, int c);
// Per-scene help-message enable flags, cleared together at scene init (the
// level sets them to show a message). Help_DisplayMessage plays the message
// sound, and the help-message renderer draws the box, only when at least one
// is set; flag1 additionally holds the fade-out for four ticks in Help_Update.
extern u8 sceneHelpMsgFlag0;
extern u8 sceneHelpMsgFlag1;

// Largest boot-asset CD stream sector count the memcard code will place in
// the level-memory stream regions; larger counts are rejected.
#define MEMCARD_STREAM_MAX_SECTORS 0x20000

// Computes the destination addresses of a boot-asset CD stream read from its
// sector count: for each of the two level-memory regions, the region base
// plus the current VU chain base minus the count. The memcard save/restore
// state machine passes bootAssets' second record size, reads the stream into
// pDest1, and uses the result's layout to place the save data.
// Symbol override: the callers in memcard.cpp still reference the
// address-based generated label for this routine.
int memcard_ComputeStreamDest(unsigned int sectors, int* pDest1, int* pDest2)
    asm("memcard_ComputeStreamDest");
int memcard_ComputeStreamDest(unsigned int sectors, int* pDest1, int* pDest2) {
    if (sectors > MEMCARD_STREAM_MAX_SECTORS) {
        *pDest1 = 0;
        *pDest2 = 0;
        return -1;
    }
    *pDest1 = levelMem.field_0x04 + currentVuChain - sectors;
    *pDest2 = levelMem.field_0x08 + currentVuChain - sectors;
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FD748);
// The Help message state machine: the current state (0-8), a frame/timeout
// counter, and per-message fields. HelpMsgCount is its +0x2C member.
struct HelpState {
    unsigned int state;      // +0x00
    int counter;             // +0x04
    int field_0x08;          // +0x08
    int field_0x0C;          // +0x0C
    int field_0x10;          // +0x10
    int field_0x14;          // +0x14
    int field_0x18;          // +0x18
    int field_0x1C;          // +0x1C
    int field_0x20;          // +0x20
    int field_0x24;          // +0x24
    int field_0x28;          // +0x28
    int msgCount;            // +0x2C
};

extern struct HelpState g_helpState __attribute__((section(".data")));

// Transitions the Help state machine based on the current state: from
// states 1-3 to 7, from 4 (counting down) and 5 to 6; 0 clears the active
// message field; 6, 7 and >= 8 leave the state unchanged.
// Symbol override: the level-overlay callers reference the address-based
// generated label for this routine.
void Help_AdvanceState(void) asm("func_001FDC08");
void Help_AdvanceState(void) {
    unsigned int state = g_helpState.state;

    if (state < 8) {
        switch (state) {
        case 0:
            g_helpState.field_0x24 = -1;
            return;

        case 1:
        case 2:
            g_helpState.state = 7;
            g_helpState.counter = 0;
            return;

        case 3:
            g_helpState.state = 7;
            g_helpState.counter = 0;
            return;

        case 4:
            g_helpState.state = 6;
            g_helpState.counter = 4 - g_helpState.counter;
            return;

        case 5:
            g_helpState.state = 6;
            g_helpState.counter = 0;
            return;

        case 6:
        case 7:
            break;
        }
    }
}
// Unreachable dead tail the original compiler emitted after
// Help_AdvanceState (func_001FDC08, matched): a stack deallocation
// (addiu sp,sp,0x10) even though the matched body takes no frame, i.e. dead
// bytes the original compiler emitted after the function's RTL, plus the
// alignment nops before Help_FindIndex. Nothing reaches it (no Ghidra
// function; tools/deadness_scan.py: 0 references), so the bytes are
// preserved with raw asm per the dead-tail policy.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001FDC90, 0x4\n"
    "glabel func_001FDC90\n"
    "    addiu $29, $29, 0x10\n"
    "endlabel func_001FDC90\n"
    "    nop\n"
    "    nop\n"
    "    nop\n"
    "    .set reorder\n"
    "    .set at\n"
);

struct HelpMsg {
    char* text;
    int id;
    int field_0x08;
    int field_0x0C;
};

extern struct HelpMsg* HelpMsgs;
extern char s_Paradox_this_message_does_not[];

// Searches HelpMsgs for the first entry whose id matches and returns its
// index, or -1 when no messages are loaded or none match.
// C linkage: the level-overlay callers reference the original unmangled
// entry point.
extern "C" int Help_FindIndex(int idx) {
    int ret = -1;
    int i = 0;
    while (i < g_helpState.msgCount) {
        if (HelpMsgs[i].id == idx) {
            ret = i;
            break;
        }
        i++;
    }
    return ret;
}

char* msg_string(int idx) {
    int i = Help_FindIndex(idx);
    if (i >= 0)
        return HelpMsgs[i].text;
    return s_Paradox_this_message_does_not;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FDD50);

// Font window for the help message: the top/bottom/left/right bounds (a
// 424x240 box centered in the lower screen), the text origin, the line
// spacing, and the print flags. Order matches the FontSetWindow parameters.
#define HELP_MSG_WIN_TOP      0xF0
#define HELP_MSG_WIN_BOT      0x1E0
#define HELP_MSG_WIN_LEFT     0x2C
#define HELP_MSG_WIN_RIGHT    0x1D4
#define HELP_MSG_TEXT_X       0x100
#define HELP_MSG_TEXT_Y       0x168
#define HELP_MSG_LINE_SPACING 0x10
#define HELP_MSG_FONT_FLAGS   7

// Message text color: 0xFFA888 at 50% alpha; the draw path reprints the same
// RGB each frame with the alpha animated from the state counter. A length of
// -1 prints to the string's null terminator.
#define HELP_MSG_TEXT_COLOR  0x80FFA888
#define HELP_MSG_TEXT_LENGTH -1

// Layout geometry stored back into g_helpState from the rendered text size
// and the draw height (occlViewParams.paramY): the box is centered on
// HELP_MSG_CENTER_X with the text HELP_MSG_TEXT_Y_FROM_BOTTOM above the draw
// bottom; if the box would come within HELP_MSG_BOTTOM_CLEARANCE of the bottom
// the text Y is clamped to keep that clearance.
#define HELP_MSG_CENTER_X            0x100
#define HELP_MSG_TEXT_Y_FROM_BOTTOM  0x3C
#define HELP_MSG_BOTTOM_CLEARANCE    0xC
#define HELP_MSG_TEXT_PAD_V          5
#define HELP_MSG_TEXT_PAD_H          10
#define HELP_MSG_BOX_INIT_HALF       8
#define HELP_MSG_CLAMP_Y_FROM_BOTTOM 0x11  // BOTTOM_CLEARANCE + TEXT_PAD_V

// Displays the current help message: sets the state to active, plays the
// message sound when a scene help flag is set, looks up the active message
// text, renders it into a FontWindow, and stores the resulting layout
// geometry (derived from the rendered text size and the draw height) back
// into the HelpState.
// Symbol override: Help_Update (the state machine) references the
// address-based generated label for this routine.
void Help_DisplayMessage(void) asm("func_001FDD58");
void Help_DisplayMessage(void) {
    g_helpState.state = 1;
    g_helpState.counter = 0;
    if (sceneHelpMsgFlag1 != 0 || sceneHelpMsgFlag0 != 0)
        func_0022DB10(0, 1, 0);

    FontWindow window;
    u8* text = (u8*)HelpMsgs[g_helpState.field_0x20].text;
    FontSetWindow(&window, HELP_MSG_WIN_TOP, HELP_MSG_WIN_BOT, HELP_MSG_WIN_LEFT,
                  HELP_MSG_WIN_RIGHT, HELP_MSG_TEXT_X, HELP_MSG_TEXT_Y,
                  HELP_MSG_LINE_SPACING, HELP_MSG_FONT_FLAGS);
    FontPrintWindowMedium(&window, HELP_MSG_TEXT_COLOR, text, HELP_MSG_TEXT_LENGTH);

    int maxTextH = window.maxTextH;
    int totalH = window.totalH;
    int base = occlViewParams.paramY;
    int bottom = base - HELP_MSG_TEXT_Y_FROM_BOTTOM;
    int offset = (totalH >> 1) + HELP_MSG_TEXT_PAD_V;
    g_helpState.field_0x08 = (maxTextH >> 1) + HELP_MSG_TEXT_PAD_H;
    g_helpState.field_0x10 = HELP_MSG_CENTER_X;
    g_helpState.field_0x0C = offset;
    g_helpState.field_0x18 = HELP_MSG_BOX_INIT_HALF;
    g_helpState.field_0x1C = HELP_MSG_BOX_INIT_HALF;
    g_helpState.field_0x14 = bottom;
    if ((base - HELP_MSG_BOTTOM_CLEARANCE) < bottom + offset) {
        int cap = (totalH >> 1) + HELP_MSG_CLAMP_Y_FROM_BOTTOM;
        g_helpState.field_0x14 = base - cap;
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/help", Help_Update);

INCLUDE_ASM("code/_generated/nonmatchings/game/help", Help_DrawPrompt);

INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FE980);

INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FECC8);
INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FED30);
INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FEE30);
