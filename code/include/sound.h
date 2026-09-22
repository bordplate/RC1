#ifndef SOUND_H
#define SOUND_H

#include "types.h"

// Per-sound 3D audio parameters. Range/volume/pitch names follow the
// Deadlocked sound module and the RC1 channel-start routine at 0x22D7F0.
struct SoundDef {
    f32 minRange; // maxVolume at this distance or less
    f32 maxRange; // minVolume at this distance or more
    int minVolume;
    int maxVolume;
    int minPitch;
    int maxPitch;
    u8 loop;
    u8 flags; // bit 0 = squared fade
    u16 index;
    int bank; // +0x1C: 989snd bank handle, initialized by startlevel
};

#endif
