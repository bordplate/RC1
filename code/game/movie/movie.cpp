#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie", func_0023A3B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie", readMpeg__FP8VideoDecP7ReadBufP7StrFile);

// C linkage: this kernel export is an unmangled SDK entry point.
extern "C" void RotateThreadReadyQueue(int);

extern "C" void switchThread() {
    RotateThreadReadyQueue(1);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie", isAudioOK);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie", initAll__Fiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie", termAll__Fv);

extern char movieErrorMessage[];
extern "C" void STUB_printf(const char* fmt, ...);

extern "C" void ErrMessage(const char* msg) {
    STUB_printf(movieErrorMessage, msg);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/movie", proceedAudio__Fv);
