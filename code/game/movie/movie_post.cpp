#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie_post", initAll__Fiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie_post", termAll__Fv);

extern char movieErrorMessage[];
// C linkage: this diagnostic wrapper is called through the original unmangled
// movie API.
extern "C" void STUB_printf(const char* fmt, ...);

// C linkage: this diagnostic wrapper is called through the original unmangled
// movie API.
extern "C" void ErrMessage(const char* msg) {
    STUB_printf(movieErrorMessage, msg);
}
