#include "common.h"
#include "audiodec.h"

void proceedAudio() {
    audioDecSend((_AudioDec*)(movieDecodeBuf + MOVIE_AUDIO_DEC_OFFSET));
}
