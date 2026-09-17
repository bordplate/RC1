#include "common.h"
#include "mobyfunc.h"
#include "mobyutil.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C5A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C658);

// Per-sound 3D audio parameters. The field names follow the Deadlocked
// sound module (sound_GetFade__FP8SoundDeffffb reads sd->minRange/maxRange/
// minVolume/maxVolume/flags, and the channel start function at 0x22D7F0
// compares sd->minPitch/maxPitch and tests sd->loop).
struct SoundDef {
    f32 minRange;   // +0x00: maxVolume at this distance or less
    f32 maxRange;   // +0x04: minVolume at this distance or more
    int minVolume;  // +0x08
    int maxVolume;  // +0x0C
    int minPitch;   // +0x10
    int maxPitch;   // +0x14
    u8 loop;        // +0x18
    u8 flags;       // +0x19: bit 0 = squared fade
    u16 index;      // +0x1A
    u32 field_0x1C; // +0x1C
};

// 3D sound channel slot (0x70 bytes; the free-slot scan in 0x22D7F0 reads
// the status byte at 0x13E5C4 + i * 0x70). The field names follow the
// Deadlocked _sound_data (Sound.channels); the offsets were verified
// against the channel start function at 0x22D7F0.
struct SoundData {
    int handle;      // +0x00
    char status;     // +0x04: 0 free, 7 active
    char flags;      // +0x05
    u8 pad_0x06[2];
    SoundDef* def;   // +0x08
    u16 index;       // +0x0C: def->index
    u16 def_index;   // +0x0E: 0xffff
    int volumeMod;   // +0x10
    int pitch;       // +0x14
    MobyInstance* pMoby; // +0x18: position is pMoby->pos at +0x10
    int field_0x1C;  // +0x1C
    vec4 pos;        // +0x20
    vec4 offset;     // +0x30
    int field_0x40;  // +0x40
    u8 pad_0x44[0x2C];
};

// Listener camera position; the first 16 bytes are the vec4 the 3D sound
// distance math reads.
extern vec4 Camera __attribute__((section(".data")));

int sound_GetFade(SoundDef* sd, float dist, float minRange, float maxRange);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", sound_GetFade__FP8SoundDeffff);

int sound_GetFade(SoundData* channel, vec4* pos) {
    float dist = FastVecDist(pos, &Camera);
    SoundDef* sd = channel->def;
    return sound_GetFade(sd, dist, sd->minRange, sd->maxRange);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C830);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C8D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", sound_update);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022D708);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022D798);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022D7F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022DA68);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022DB10);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022DBA0);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022DC38);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", sound_StopAllSounds__Fv);

void sound_setBufferValue(int a, long b) {
    int c = (int)b;
    if (c)
        *(int*)c = a;
}

void sound_setBufferState(int a, long b) {
    int c = (int)b;
    if (c == 0) return;
    *(int*)c = a;
    if (a != 0) {
        if (*(u8*)(c + 4) != 1) return;
        *(u8*)(c + 4) = 2;
        return;
    }
    *(int*)(c + 0x18) = 0;
    *(int*)(c + 0x1C) = 0;
    *(u8*)(c + 4) = 0;
}

void sound_resetBufferState(int a, long b) {
    int c = (int)b;
    if (c == 0) return;
    *(int*)c = a;
    if (a != 0) return;
    *(int*)(c + 0x18) = 0;
    *(int*)(c + 0x1C) = 0;
    *(char*)(c + 4) = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022DE08);
