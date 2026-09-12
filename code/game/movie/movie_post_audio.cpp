#include "common.h"
#include "audiodec.h"

// Audio-decoder callback installed by the movie-decoder API: it sends the
// movie _AudioDec state to the SPU. This TU needs -mno-split-addresses so
// the movieDecodeBuf load stays a single self-based pseudo-instruction
// hoisted before the prologue, as in the original (the flag breaks
// ErrMessage's matched codegen in movie_post.cpp).
void proceedAudio() {
    audioDecSend((_AudioDec*)(movieDecodeBuf + MOVIE_AUDIO_DEC_OFFSET));
}
