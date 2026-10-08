#include "common.h"
#include "types.h"

// Freeze/pause dialog state @0x193300 (named in config/symbols.txt). The
// original stores the 32-bit message-text addresses (msg_string results) with
// sw, so query/ans1/ans2 are u32, not true 64-bit pointers. Layout is known
// through offset 0x28; the last four fields are unconfirmed.
typedef struct {
    u32 mode;
    u32 countdown;
    u32 query;
    u32 ans1;
    u32 ans2;
    u32 prevGameMode;
    u32 restorePage;
    u32 field_0x1C;
    u32 field_0x20;
    u32 field_0x24;
    u32 field_0x28;
} freeze_t;
extern freeze_t Freeze;

extern u32 GameMode;
// C linkage: defined in the C sound-library TU (989snd_post.c), so the symbol
// is unmangled. Pauses every sound in the given group.
extern "C" void snd_PauseAllSoundsInGroup(int group);
char* msg_string(int msgId);
void music_Pause(int arg);
// C linkage: handwritten fast function in game/fastfunc; scales a frame count
// by the frame-rate factor (see bmain.cpp).
extern "C" int func_001F96F8(int frames);
// C linkage: the level-overlay callers reference the original unmangled
// entry point. Marks the message id as the most recently active (see
// help.cpp) so the pause/help screen lists shown messages in recency order.
extern "C" void Help_TouchMessage(int msgId);

// Freeze countdown duration in frames (scaled by func_001F96F8).
#define FREEZE_COUNTDOWN_FRAMES 0x1E
// Default-case countdown: stored raw, not frame-rate scaled. 120 frames.
#define FREEZE_DEFAULT_COUNTDOWN_FRAMES 0x78
// Bitmask of sound groups paused when entering the freeze. The IOP command
// takes a group bitmask; the per-bit meanings are not established.
#define FREEZE_SOUND_GROUPS 0x1D

// Message-table ids for msg_string / Help_TouchMessage. The table is loaded at
// runtime per level (Help_LoadMsgs over help_messages_<lang>.bin; the global
// set in assets/globals/all_text.bin); the id space is shared across levels
// and languages, and the glyph bytes below (0x10/0x11/0x12) select button
// symbols from the font texture row, independent of the text. Texts (US
// English, from assets/globals/all_text.bin):
//   MSG_QUIT_RACE                "Quit Race?"
//   MSG_QUIT                     "Quit?"
//   MSG_BTN_QUIT                 "<circle glyph> Quit"
//   MSG_BTN_CONTINUE_CROSS       "<cross glyph> Continue"
//   MSG_BTN_CONTINUE_CIRCLE      "<circle glyph> Continue"
//   MSG_BTN_EXIT                 "<triangle glyph> Exit"
//   MSG_AUTOSAVE_WARNING         "When this icon appears, your progress is
//                                 being saved. ... do not remove the Memory
//                                 Card (PS2) or turn off the power."
#define MSG_QUIT_RACE 0x4F6E
#define MSG_QUIT 0x5229
#define MSG_BTN_QUIT 0x5248
#define MSG_BTN_CONTINUE_CROSS 0x5249
#define MSG_BTN_CONTINUE_CIRCLE 0x524A
#define MSG_BTN_EXIT 0x4EE0
#define MSG_AUTOSAVE_WARNING 0x4E2B

void mode_freezeInit(int dialog, int restorePage)
{
    if (GameMode != 3) {
        snd_PauseAllSoundsInGroup(FREEZE_SOUND_GROUPS);
        music_Pause(0);
    }

    Freeze.prevGameMode = GameMode;
    Freeze.restorePage = restorePage;
    GameMode = 4;
    Freeze.mode = dialog;

    switch (dialog) {
    case 0:
        Freeze.query = (u32)msg_string(MSG_QUIT_RACE);
        Freeze.ans1 = (u32)msg_string(MSG_BTN_QUIT);
        Freeze.ans2 = (u32)msg_string(MSG_BTN_CONTINUE_CROSS);
        Freeze.countdown = 0;
        Freeze.field_0x1C = 0;
        Freeze.field_0x20 = 0;
        Freeze.field_0x24 = 0;
        Freeze.field_0x28 = 0;
        break;
    case 2:
        Freeze.query = (u32)msg_string(MSG_BTN_CONTINUE_CIRCLE);
        Freeze.ans1 = 0;
        Freeze.countdown = 0;
        break;
    case 1:
    case 4:
        Freeze.query = (u32)msg_string(MSG_QUIT);
        Freeze.ans1 = (u32)msg_string(MSG_BTN_EXIT);
        Freeze.ans2 = (u32)msg_string(MSG_BTN_CONTINUE_CIRCLE);
        Freeze.countdown = 0;
        break;
    case 5:
        Help_TouchMessage(MSG_AUTOSAVE_WARNING);
        Freeze.countdown = func_001F96F8(FREEZE_COUNTDOWN_FRAMES);
        Freeze.field_0x20 = 0;
        Freeze.field_0x24 = func_001F96F8(FREEZE_COUNTDOWN_FRAMES);
        break;
    case 3:
        Freeze.countdown = func_001F96F8(FREEZE_COUNTDOWN_FRAMES);
        Freeze.field_0x20 = 0;
        Freeze.field_0x24 = func_001F96F8(FREEZE_COUNTDOWN_FRAMES);
        break;
    case 6:
        Freeze.countdown = func_001F96F8(FREEZE_COUNTDOWN_FRAMES);
        Freeze.field_0x1C = 0;
        break;
    default:
        Freeze.ans1 = 0;
        Freeze.countdown = FREEZE_DEFAULT_COUNTDOWN_FRAMES;
        Freeze.ans2 = 0;
        Freeze.query = 0;
        break;
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/freeze", DrawDialogText__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/freeze", UpdateModeFreeze__Fv);
