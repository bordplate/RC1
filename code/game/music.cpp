#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/music", func_00215390);

void stream_updateCdStatus(int error);

// C linkage: the sound-library callback registration function is an unmangled
// entry point supplied by generated assembly.
extern "C" void snd_StreamSafeCdCallback(void (*callback)(int));

void music_registerCdCallback(void) {
    snd_StreamSafeCdCallback(stream_updateCdStatus);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/music", func_00215440);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", func_00215518);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", func_00215600);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", func_002156D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", func_002157D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", func_002158A0);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", func_00215970);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", func_00215B10);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", music_PreseekTrack__Fiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", music_StartTrack__Fiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", music_StartTrackBody__Fiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", music_Transition__Fiiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", music_Stop__Fv);

typedef struct MusicState {
    u8 pad0_40[0x40];
    s16 field_0x40;
    s16 field_0x42;
    u8 pad44_5c[0x18];
    s16 field_0x5C;
    s16 field_0x5E;
    u8 pad60_78[0x18];
    s16 field_0x78;
    s16 field_0x7A;
} MusicState;

extern MusicState musicTransition __attribute__((section(".data")));

void music_Pause(int pauseStream) {
    if (pauseStream != 0) {
        musicTransition.field_0x5C = -0x8000;
        musicTransition.field_0x5E = 0;
    }
    musicTransition.field_0x40 = -0x8000;
    musicTransition.field_0x42 = 0;
    musicTransition.field_0x78 = -0x8000;
    musicTransition.field_0x7A = 0;
}

class music {
public:
    // Symbol override: the original free-style entry point is named with the
    // legacy music_Unpause label rather than this class member's natural name.
    void Unpause() asm("music_Unpause__Fv");
};

void music::Unpause() {
    musicTransition.field_0x40 = 4;
    musicTransition.field_0x78 = 4;
    musicTransition.field_0x5C = 4;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/music", music_UpdateStream__FR13music_Playing);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", music_Update__Fv);
