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

// The prompt box/icon geometry and fade: the prompt is a 64x64 effect-texture
// icon centered in the help box (field_0x10/field_0x14), drawn with a gray
// alpha tint, plus a beveled UI frame around the box. Its alpha fades in by
// HELP_PROMPT_FADE_STEP per counter tick while the state machine is in a fade
// state, clamped to full opacity; otherwise it sits just below full.
#define HELP_PROMPT_BOX_HALF   0x20  // prompt box/icon half-extent (64x64 box)
#define HELP_PROMPT_TEX_SIZE   0x40  // prompt icon size and texture region
#define HELP_PROMPT_FRAME_ALPHA 0x60 // UI frame opacity behind the prompt
#define HELP_PROMPT_EFFECT_TEX 4     // effectTexs[] slot bound for the icon
#define HELP_PROMPT_TINT_RGB   0x808080 // gray RGB tint of the prompt icon
#define HELP_PROMPT_FADE_STEP  21    // alpha gained per counter tick fading
#define HELP_PROMPT_ALPHA_IDLE 0x7E  // prompt alpha outside a fade state
#define HELP_PROMPT_ALPHA_FULL 0x80  // full opacity; the fade clamps to this

// GetEffectTex returns the 64-bit tex0 word of effectTexs[texId] (a 64-bit ld
// in its epilogue), so the return must be 64-bit for the icon argument below.
// The real entry takes a second int this call site leaves stale (only a0 is
// set before the jal); this EGC rejects a one-argument call to a two-parameter
// prototype, so the one-parameter form is pinned to the original label.
unsigned long GetEffectTex(int texId) asm("GetEffectTex__Fii");
// C linkage: the stripped-ELF symbol is unmangled DrawTexturedQuad, in the
// same C-linkage UI/draw API family as DrawUIFrame (whose definition in
// draw_post_post.cpp carries the full evidence).
// Draws a textured quad at (x, y) of size (w, h) sampling the texture region
// (u, v) of size (uw, vh), with a 64-bit color and a 64-bit texture reference.
extern "C" void DrawTexturedQuad(int x, int y, int w, int h, int u, int v,
                                 int uw, int vh, unsigned long color,
                                 unsigned long tex);
// C linkage: confirmed unmangled in the stripped ELF; the definition with the
// full evidence comment lives in draw_post_post.cpp (same UI/draw family).
extern "C" void DrawUIFrame(int top, int bot, int left, int right, int alpha);

// Draws the prompt: a 64x64 effect-texture icon centered in the help box with
// a gray alpha tint, over a beveled UI frame around the box. The box half is
// stored in the HelpState (field_0x18/field_0x1C) for the rest of the prompt
// rendering. The icon's alpha fades in with the counter while the state
// machine is in a fade state (1 or 7), clamped to full opacity, and otherwise
// sits just below full.
// C linkage: the stripped-ELF symbol is unmangled Help_DrawPrompt; the
// level-overlay callers reference the original unmangled entry point (same
// as the sibling Help_FindIndex in this file).
extern "C" void Help_DrawPrompt(void) {
    g_helpState.field_0x18 = HELP_PROMPT_BOX_HALF;
    g_helpState.field_0x1C = HELP_PROMPT_BOX_HALF;
    DrawUIFrame(g_helpState.field_0x14 - HELP_PROMPT_BOX_HALF,
                g_helpState.field_0x14 + HELP_PROMPT_BOX_HALF,
                g_helpState.field_0x10 - HELP_PROMPT_BOX_HALF,
                g_helpState.field_0x10 + HELP_PROMPT_BOX_HALF,
                HELP_PROMPT_FRAME_ALPHA);
    int alpha;
    if (g_helpState.state == 1 || g_helpState.state == 7)
        alpha = g_helpState.counter * HELP_PROMPT_FADE_STEP;
    else
        alpha = HELP_PROMPT_ALPHA_IDLE;
    if (alpha > HELP_PROMPT_ALPHA_FULL)
        alpha = HELP_PROMPT_ALPHA_FULL;
    unsigned long tex = GetEffectTex(HELP_PROMPT_EFFECT_TEX);
    unsigned long color = (alpha << 24) | HELP_PROMPT_TINT_RGB;
    DrawTexturedQuad(g_helpState.field_0x10 - HELP_PROMPT_BOX_HALF,
                     g_helpState.field_0x14 - HELP_PROMPT_BOX_HALF,
                     HELP_PROMPT_TEX_SIZE, HELP_PROMPT_TEX_SIZE, 0, 0,
                     HELP_PROMPT_TEX_SIZE, HELP_PROMPT_TEX_SIZE, color, tex);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FE980);

// A message-id / text-code pair from the message table. The code is the
// sequential text handle (0x526F+) the renderer uses to fetch the string for
// a message id; the table lets the game map between the two spaces.
struct MessageIdCode {
    s16 id;
    u16 code;
};
// 150 message-id / text-code pairs (the final two are zero); the id space is
// shared across levels and languages. Sits in .data right before the
// "Paradox: This message does not exist" fallback string.
extern struct MessageIdCode messageIdCodeTable[150];
// Records in messageIdCodeTable; each record is 2 s16 (id, code) = 4 bytes.
#define MSG_ID_CODE_TABLE_RECORDS 150
#define MSG_ID_CODE_RECORD_S16 2
#define MSG_ID_CODE_RECORD_BYTES 4

// Finds the record in messageIdCodeTable whose id (column 0) or code
// (column 1) equals value and returns its index, or -1 when nothing matches.
// A code search (column 1) with a non-null idOut also stores the record's id
// there, mapping a text-code back to its message id. func_001FED30 searches
// by message id to track which messages are shown; the pause screen searches
// by code to recover the id.
// Codegen notes (required to match this EGC's tie-breaks): the pre-loop
// barrier pins the table base / search pointer / offset into t2 / t0 / a3;
// the loop iterator's shift lands in v1 via the pinned r; the store address
// is an integer add (q + base) so the base stays the right operand; and the
// empty asm keeps i++ out of the exit-branch delay slot.
// Symbol override: the callers still reference the address-based label.
int messageIdCodeTableFind(int value, int column, u16* idOut)
    asm("func_001FECC8");
int messageIdCodeTableFind(int value, int column, u16* idOut) {
    value = (s16)value;
    s16* base = (s16*)messageIdCodeTable;
    s16* p = base + column;
    int i = 0;
    int q;
    asm volatile("" : : "r"(p), "r"(base) : "$3", "$7");
    q = sizeof(s16);
    do {
        if (*p == value) {
            register int r asm("$3") = i * MSG_ID_CODE_RECORD_BYTES;
            q = column ? r : q;
            if (idOut)
                *idOut = *(u16*)(q + (u32)base);
            return i;
        }
        asm volatile("");
        i++;
        q += MSG_ID_CODE_RECORD_BYTES;
        p += MSG_ID_CODE_RECORD_S16;
    } while (i < MSG_ID_CODE_TABLE_RECORDS);
    return -1;
}

// Records in messageIdCodeTable, and the indices of those that are currently
// shown, in recency order (most recent last); the pause/help screen builds its
// sprite list from the indices. One byte per index, max
// MSG_ID_CODE_TABLE_RECORDS entries.
extern u8 activeMessageRecords[];
extern int activeMessageRecordCount;

// Marks the message id's table record as the most recently active: if the
// record index is already in activeMessageRecords it is shifted to the end,
// otherwise it is appended. Shown messages are touched when they are raised
// (e.g. the autosave warning), so the list tracks recency. A message id that
// is not in the table is ignored.
// C linkage: the entry point is referenced by callers with an unmangled C
// label, as in the sibling Help_FindIndex (the independent Lombyte analysis
// of this same build declares it C-style too).
// Codegen notes (required to match this EGC's tie-breaks): the search must be
// a for loop whose iterator increment sits in the clause (a while with i++ in
// the body drops the dead count load in the blez delay slot); the shift must
// be a while with i++ in the body and direct array indexing on the global (a
// pointer local triggers EGC's countdown transform and a for loop re-pipelines
// it).
extern "C" void Help_TouchMessage(int msgId) {
    int rec = messageIdCodeTableFind((s16)msgId, 0, (u16*)0);
    if (rec == -1)
        return;
    int i = 0;
    for (; activeMessageRecords[i] != rec && i < activeMessageRecordCount; i++)
        ;
    if (i < activeMessageRecordCount) {
        while (i < activeMessageRecordCount - 1) {
            activeMessageRecords[i] = activeMessageRecords[i + 1];
            i++;
        }
        activeMessageRecords[i] = 0;
        activeMessageRecordCount = activeMessageRecordCount - 1;
    }
    int c = activeMessageRecordCount;
    activeMessageRecords[c] = (u8)rec;
    activeMessageRecordCount = c + 1;
}

// Unreachable dead tail the original compiler emitted after Help_TouchMessage:
// a stack deallocation (addiu sp,sp,0x10) plus the alignment nop before the
// next function. Nothing reaches it (no Ghidra function;
// tools/deadness_scan.py: 0 references), so the bytes are preserved with raw
// asm per the dead-tail policy.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001FEE30, 0x4\n"
    "glabel func_001FEE30\n"
    "    addiu $29, $29, 0x10\n"
    "endlabel func_001FEE30\n"
    "    nop\n"
    "    .set reorder\n"
    "    .set at\n"
);
