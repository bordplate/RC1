#ifndef AUDIODEC_H
#define AUDIODEC_H

#include "types.h"

// Movie audio decoder state (the `state` member of _AudioDec). Values are
// verified from the boot ELF state tests (audioDecBeginPut and audioDecEndPut
// take the state == 0 first-put/header paths, audioDecSend gates on state != 0,
// and sendADPCM branches on states 1, 2, and 3) and match the direct
// descendant Deadlocked reference (reference/dl/game_dl/movie/audiodec.cpp).
enum {
    // Created or reset; no stream data buffered yet.
    AUDIODEC_STATE_IDLE = 0,
    // Fully buffered; the send path starts once 0x1000 bytes are in.
    AUDIODEC_STATE_BUFFERED = 1,
    // Started by audioDecStart; the send path tracks the SPU via
    // snd_GetMovieNAX.
    AUDIODEC_STATE_STARTED = 2,
    // Stream finished; the send path returns without sending. The write
    // site is unconfirmed.
    AUDIODEC_STATE_DONE = 3,
};

// Movie audio IOP DMA block size (0x400 bytes): audioDecStart passes the
// buffer size to snd_StartMovieSound rounded down to a multiple of this.
// The shift form in audioDecStart is codegen-locked to a bare sra/sll pair,
// so the log2 of the block size is named alongside it.
#define AUDIODEC_IOP_BLOCK_BITS 10
#define AUDIODEC_IOP_BLOCK_SIZE (1 << AUDIODEC_IOP_BLOCK_BITS)

// Audio decoder state block inside the movie decode buffer. Every observed
// member is a 4-byte int, so only the unobserved gaps are pad.
// audioDecReset zeros the listed int fields; the sibling generated functions
// (audioDecCreate, audioDecStart, audioDecBeginPut, audioDecEndPut, sendToSPU,
// sendADPCM) establish the remaining offsets.
typedef struct _AudioDec {
    int state;
    int field_0x04;
    u8 pad_08[0xC];
    int field_0x14;
    int field_0x18;
    int field_0x1C;
    u8 pad_20[0x10];
    int field_0x30;
    int field_0x34;
    int field_0x38;
    int field_0x3C;
    int field_0x40;
    int field_0x44;
    int field_0x48;
    int field_0x4C;
    int sentPos;
    u8 pad_54[4];
    int field_0x58;
    int field_0x5C;
    int field_0x60;
} _AudioDec;

// movieDecodeBuf holds the movie decode buffer base, written by the movie
// playback entry point (func_0023A3B8) and zeroed on its exit. It sits
// inside the gp window, so a plain extern would compile to a single
// GPREL16 load; the original uses an absolute self-based lui/lw, which the
// .data section attribute forces. The consuming TUs (movie_mid,
// videodec_post, videodec_nodata) compile with -mno-split-addresses so the
// load stays a single pseudo instruction and the prologue schedules around
// it as in the original.
extern u8* movieDecodeBuf __attribute__((section(".data")));

// Offset of the movie video ViBuf inside the movie decode buffer (the
// movieDecodeBuf base).
#define MOVIE_VIBUF_OFFSET 0xD9090

// Offset of the _AudioDec state inside the movie decode buffer (the
// movieDecodeBuf base).
#define MOVIE_AUDIO_DEC_OFFSET 0xD9100

// C linkage: these decoder callbacks are referenced by the original unmangled
// movie-decoder API.
extern "C" int audioDecIsPageFull(_AudioDec* self);
extern "C" void audioDecSend(_AudioDec* self);

#endif
