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
