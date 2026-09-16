#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/disp", setImageTag);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/disp", vblankHandler);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/disp", handler_endimage);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/disp", startDisplay__Fi);

// Movie display active flag: startDisplay__Fi sets it after waiting for the
// display hardware state, vblankHandler gates frame counting on it, and
// endDisplay clears it. It sits inside the gp window, so a plain extern
// would compile to a single GPREL16 store; the original uses an absolute
// self-based lui/sw, which the .data section attribute forces.
extern int movieDisplayActive __attribute__((section(".data")));

void endDisplay(void) {
    movieDisplayActive = 0;
}
