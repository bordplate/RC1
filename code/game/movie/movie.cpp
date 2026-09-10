#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie", func_0023A3B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie", readMpeg__FP8VideoDecP7ReadBufP7StrFile);

// C linkage: this kernel export is an unmangled SDK entry point.
extern "C" void RotateThreadReadyQueue(int);

// C linkage: this wrapper is called by generated movie code through its
// unmangled kernel-style entry point.
extern "C" void switchThread() {
    RotateThreadReadyQueue(1);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie", isAudioOK);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie", initAll__Fiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie", termAll__Fv);

extern char movieErrorMessage[];
// C linkage: this diagnostic wrapper is called through the original unmangled
// movie API.
extern "C" void STUB_printf(const char* fmt, ...);

// C linkage: this diagnostic wrapper is called through the original unmangled
// movie API.
extern "C" void ErrMessage(const char* msg) {
    STUB_printf(movieErrorMessage, msg);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie", proceedAudio__Fv);
