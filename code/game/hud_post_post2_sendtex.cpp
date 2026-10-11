#include "common.h"
#include "types.h"
#include "hud.h"
#include "sce_gs.h"

// Double volatile: EGC must re-read the VU1 chain head pointer before every
// packet store; a plain or single-volatile declaration lets it CSE the later
// loads into one shared base. This TU is assembled with ps2eeas, which expands
// each bare `lw r, vu1ChainHead` pseudo into a fresh self-based lui/lw pair,
// matching the original. (A .data attribute or a GNU-assembled TU breaks that.)
extern volatile u32* volatile vu1ChainHead;
// Same object as vu1ChainHead: the plain alias keeps the GPREL form of the
// head updates and of the first store of the post-call record, all of which
// sit in branch/bnez delay slots.
extern volatile u32* vu1ChainHeadStore;

// VIF packet tags for the two VU1 chain packets this function appends. A packet
// is [start | qcnt, payload, end | qcnt], where qcnt is the payload length in
// 16-byte blocks. The end tag 0x50000000 is shared by both packets; the start
// tag is 0x10000000 for the inline load-image record and 0x30000000 for the
// data reference to the texture source (see framebuf.cpp / vuchain.cpp).
#define VU1_INLINE_PKT_TAG 0x10000000u
#define VU1_DATA_REF_TAG 0x30000000u
#define VU1_DATA_REF_END_TAG 0x50000000u

// Appends a 96-byte load-image def record (filled by sceGsSetDefLoadImage) and
// a data reference to `src` to the VU1 chain, or, when `immediate` is set,
// transfers the texture straight from a stack copy via sceGsExecLoadImage.
void Hud_sendTexture(char* src, int dest, int format, int uLog, int vLog,
                     int immediate) {
    sceGsLoadImage lregs;
    sceGsLoadImage* image;
    int qwc;
    int tbw;
    int t;
    const int recQcnt = sizeof(sceGsLoadImage) / 16;  // 6: the 96-byte record

    t = 1 << uLog;
    tbw = t >> 6;
    qwc = 1 << (uLog + vLog - 4);
    tbw = tbw > 0 ? tbw : 1;
    if (immediate == 0) {
        vu1ChainHead[0] = VU1_INLINE_PKT_TAG | recQcnt;
        vu1ChainHead[1] = 0;
        vu1ChainHead[2] = 0;
        vu1ChainHead[3] = VU1_DATA_REF_END_TAG | recQcnt;
        u32* head = (u32*)vu1ChainHead;
        vu1ChainHeadStore = head + 0x1C;
        image = (sceGsLoadImage*)(head + 4);
    } else {
        image = &lregs;
    }
    sceGsSetDefLoadImage(image, dest, tbw, format, 0, 0, 1 << uLog, 1 << vLog);
    if (immediate == 0) {
        vu1ChainHeadStore[0] = VU1_DATA_REF_TAG | qwc;
        vu1ChainHead[1] = (u32)src;
        vu1ChainHead[2] = 0;
        vu1ChainHead[3] = VU1_DATA_REF_END_TAG | qwc;
        vu1ChainHeadStore = vu1ChainHead + 4;
    } else {
        FlushCache(0);
        sceGsExecLoadImage(image, src);
    }
}
