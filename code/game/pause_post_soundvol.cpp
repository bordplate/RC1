#include "common.h"
#include "types.h"

// Pause sound-options callback (item table 0x1D1578 slot 3): refreshes the
// music-volume slot copy pauseSoundVolume (0x13E5A0) from the raw option
// musicVolumeRaw, scaled to 80%. The raw option is stepped by +/-3 in
// SoundOptionsMenu, clamped to [0, 1024]; the snd init at 0x22C8D0 derives
// the same 80% and 70% slot values.
//
// This TU compiles with the project default -G8 (no -G0): the small-data
// classification of the plain int extern makes EGC emit a single bare
// "lw v1, musicVolumeRaw" pseudo that ps2eeas expands in-place to the
// original self-based absolute lui/lw pair. Under -G0 (or -G2) EGC splits
// the load into a two-register absolute pair and interleaves "li a0,10"
// between the lui and lw, which does not match. The sibling TU
// pause_post2.o keeps -G0 because pause_resetMenuEntry's inlined PI float
// constant requires it. See notes/pause_func_0021CB00.md.
extern int musicVolumeRaw;
extern int pauseSoundVolume __attribute__((section(".data")));

int pause_updateSoundVolume(void) {
    pauseSoundVolume = musicVolumeRaw * 8 / 10;
    return 0;
}
