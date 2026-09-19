#include "common.h"

extern void snd_SendIOPCommandAndWait(int cmd, int count, void* data);
extern void snd_SendIOPCommandNoWait(int cmd, int count, void* data, int x, int y);

INCLUDE_ASM("code/_generated/nonmatchings/989snd/ee/989snd_pre", snd_StartSoundSystem);
