#include "common.h"
#include "types.h"
#include "audiodec.h"

// movieDecodeBuf holds the movie decode buffer base, written by the movie
// playback entry point (func_0023A3B8) and zeroed on its exit. It sits
// inside the gp window, so a plain extern would compile to a single
// GPREL16 load; the original uses an absolute self-based lui/lw, which the
// .data section attribute forces. With this TU's -mno-split-addresses the
// load stays a single pseudo-instruction and the compiler schedules the
// prologue around it as in the original.
extern u8* movieDecodeBuf __attribute__((section(".data")));

// C linkage: this unmangled movie API wrapper queries the audio decoder
// state in the movie buffer.
extern "C" int isAudioOK() {
    return audioDecIsPageFull((_AudioDec*)(movieDecodeBuf + MOVIE_AUDIO_DEC_OFFSET));
}
