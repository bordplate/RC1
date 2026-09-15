#include "common.h"
#include "types.h"
#include "vibuf.h"
#include "audiodec.h"

int viBufRestartDMA(ViBuf* buf);
int viBufStopDMA(ViBuf* buf);

// Video-decoder callback that stops the movie video ViBuf DMA, the stop
// counterpart of mpegRestartVideoDMA below. Same -mno-split-addresses
// movieDecodeBuf load.
int mpegStopVideoDMA() asm("func_0023D0E0");

int mpegStopVideoDMA() {
    viBufStopDMA((ViBuf*)(movieDecodeBuf + MOVIE_VIBUF_OFFSET));
    return 1;
}

// Video-decoder callback (table slot 3) installed by videoDecCreate: it
// restarts the movie video ViBuf DMA. It ignores its callback arguments.
// This TU needs -mno-split-addresses so the movieDecodeBuf load stays a
// single pseudo-instruction hoisted before the prologue, as in the original.
int mpegRestartVideoDMA() {
    viBufRestartDMA((ViBuf*)(movieDecodeBuf + MOVIE_VIBUF_OFFSET));
    return 1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec_post", func_0023D140);
