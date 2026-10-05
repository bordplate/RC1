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
// C linkage: implemented in the generated nonmatching help assembly
// (func_001FED30.s), whose entry point is the unmangled label. Shows a freeze
// dialog prompt for the given message id; purpose otherwise unconfirmed.
extern "C" void func_001FED30(int msgId);

// Freeze countdown duration in frames (scaled by func_001F96F8).
#define FREEZE_COUNTDOWN_FRAMES 0x1E

void mode_freezeInit(int dialog, int restorePage)
{
    if (GameMode != 3) {
        snd_PauseAllSoundsInGroup(0x1D);
        music_Pause(0);
    }

    Freeze.prevGameMode = GameMode;
    Freeze.restorePage = restorePage;
    GameMode = 4;
    Freeze.mode = dialog;

    switch (dialog) {
    case 0:
        Freeze.query = (u32)msg_string(0x4F6E);
        Freeze.ans1 = (u32)msg_string(0x5248);
        Freeze.ans2 = (u32)msg_string(0x5249);
        Freeze.countdown = 0;
        Freeze.field_0x1C = 0;
        Freeze.field_0x20 = 0;
        Freeze.field_0x24 = 0;
        Freeze.field_0x28 = 0;
        break;
    case 2:
        Freeze.query = (u32)msg_string(0x524A);
        Freeze.ans1 = 0;
        Freeze.countdown = 0;
        break;
    case 1:
    case 4:
        Freeze.query = (u32)msg_string(0x5229);
        Freeze.ans1 = (u32)msg_string(0x4EE0);
        Freeze.ans2 = (u32)msg_string(0x524A);
        Freeze.countdown = 0;
        break;
    case 5:
        func_001FED30(0x4E2B);
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
        Freeze.countdown = 0x78;
        Freeze.ans2 = 0;
        Freeze.query = 0;
        break;
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/freeze", DrawDialogText__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/freeze", UpdateModeFreeze__Fv);
