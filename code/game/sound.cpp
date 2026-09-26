#include "common.h"
#include "camera.h"
#include "mobyfunc.h"
#include "mobyutil.h"
#include "types.h"
#include "sound.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C5A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C658);

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

int sound_GetFade(SoundDef* sd, float dist, float minRange, float maxRange);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", sound_GetFade__FP8SoundDeffff);

int sound_GetFade(SoundData* channel, vec4* pos) {
    // Listener position: the 16-byte camera position quad (currentCamera.pos).
    float dist = FastVecDist(pos, (vec4*)&currentCamera.pos);
    SoundDef* sd = channel->def;
    return sound_GetFade(sd, dist, sd->minRange, sd->maxRange);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C830);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", func_0022C8D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", sound_update);

INCLUDE_ASM("code/_generated/nonmatchings/game/sound", sound_loadBankByLocation__Fi);

// The 0x70-byte sound channel slots start at audioState+0x70 (see SoundData).
// This routine indexes the slots from the audioState base with a 0x70 byte
// stride, so every field offset is +0x70 relative to SoundData (status
// 0x04->0x74, pMoby 0x18->0x88, field_0x1C->0x8C).
struct SoundChannelView {
    u8 pad_0x00[0x74];
    u8 status;         // +0x74: 0 free, 4 pending-kill, 6 ?, 7 active
    u8 pad_0x75[0x13];
    void* pMoby;       // +0x88
    void* pAmbient;    // +0x8C
};
extern u8 audioState[];

void sound_KillChannel(int i) {
    if (i < 0) return;
    SoundChannelView* ch = (SoundChannelView*)((u8*)audioState + i * 0x70);
    u8 status = ch->status;
    if (status == 7) {
        ch->pMoby = 0;
        ch->pAmbient = 0;
        ch->status = 0;
    } else if (status != 0 && status != 6) {
        ch->status = 4;
    }
}

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
