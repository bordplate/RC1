#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/music", func_00215390);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", func_00215420);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/music", music_Pause__Fi);

typedef struct MusicState {
    u8 pad0_40[0x40];
    u16 field_0x40;
    u8 pad42_5c[0x1A];
    u16 field_0x5C;
    u8 pad5e_78[0x1A];
    u16 field_0x78;
} MusicState;

extern MusicState D_001516D0 __attribute__((section(".data")));

class music {
public:
    void Unpause() asm("music_Unpause__Fv");
};

void music::Unpause() {
    D_001516D0.field_0x40 = 4;
    D_001516D0.field_0x78 = 4;
    D_001516D0.field_0x5C = 4;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/music", music_UpdateStream__FR13music_Playing);

INCLUDE_ASM("code/_generated/nonmatchings/game/music", music_Update__Fv);
