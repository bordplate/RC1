#include "common.h"
#include "types.h"
#include "vibuf.h"
#include "audiodec.h"

struct sceMpeg;
struct sceMpegCbData;

void viBufAddDMA(ViBuf* buf);
// C linkage: this kernel routine is an unmangled generated entry point.
extern "C" void switchThread(void);

// Video-decoder callback (table slot 1) installed by videoDecCreate: it
// yields the thread, then arms the movie video ViBuf DMA. It ignores its
// callback arguments. This TU needs -mno-split-addresses so the
// movieDecodeBuf load stays a single self-based pseudo-instruction, as in
// the original.
int mpegNodata(struct sceMpeg* mpeg, struct sceMpegCbData* cbData, void* user) {
    switchThread();
    viBufAddDMA((ViBuf*)(movieDecodeBuf + MOVIE_VIBUF_OFFSET));
    return 1;
}
