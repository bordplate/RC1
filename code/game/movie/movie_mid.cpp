#include "common.h"
#include "types.h"
#include "audiodec.h"

// C linkage: this unmangled movie API wrapper queries the audio decoder
// state in the movie buffer.
extern "C" int isAudioOK() {
    return audioDecIsPageFull((_AudioDec*)(movieDecodeBuf + MOVIE_AUDIO_DEC_OFFSET));
}
