#ifndef SKY_H
#define SKY_H

#include "types.h"

// One sky shell layer. The dispatch (SkyDrawShell) reads only the draw-mode
// flag; the rest of the block is consumed by the shell draw functions.
struct sky_shell {
    u8 pad_00[4];
    u32 flags; // 0x04: 0 = textured draw, nonzero = Gouraud draw
};

// Per-level sky state block pointed to by the Sky global.
struct SKY {
    u8 pad_00[6];
    s16 shellCnt; // 0x06: shell layer count (compared signed)
    u8 pad_08[0x18];
    sky_shell* shells[1]; // 0x20: shell layer table
};

extern SKY* Sky;

void SkyDrawShell(int ishell);

// C linkage: the recovered original symbol name is unmangled while the C++
// siblings in the same file (SkyDrawShell__Fi, SetupSkyGifPaging__Fv) are mangled.
extern "C" void SkyDrawShellTextured(sky_shell* shell);
// C linkage: the recovered original symbol name is unmangled while the C++
// siblings in the same file (SkyDrawShell__Fi, SetupSkyGifPaging__Fv) are mangled.
extern "C" void SkyDrawShellGouraud(sky_shell* shell);

#endif
