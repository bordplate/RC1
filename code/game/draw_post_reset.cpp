#include "common.h"
#include "types.h"
#include "video.h"

extern int spaceLoadInProgress;
// textureCursorEnd mixes address modes in the original: the save is a
// self-based absolute load (the .data alias below) while the restore is a
// GP-relative store through the seeded alias (config/linker_aliases.ld),
// mirroring camera.cpp's screenFade/screenFadeGp pair. Both map to 0x15EE78.
extern int textureCursorEndGp;
asm(".extern textureCursorEndGp, 4");
// .data-classified aliases (config/linker_aliases.ld) at the same addresses
// as vifChainFlags (0x160EE0) and textureCursorEnd (0x15EE78). Under this TU's
// -mno-split-addresses they lower to an unsplittable self-based absolute
// pseudo that ps2eeas expands in place to the original's two-instruction pair.
extern int resetVideoVifChainFlagsAbs __attribute__((section(".data")));
extern int resetVideoTextureCursorEndAbs __attribute__((section(".data")));
// Same VU1 chain head as vuchain's vu1ChainHead, declared plain so the reset
// writes the low word with a GP-relative store (see vuchain.cpp).
extern volatile u32* vu1ChainHeadStore;

// VU1_syncChain's sit-and-spin timeout recovery: a full reset of the VIF1,
// DMAC, GS and VU1 video pipeline. The texture cursor end is saved across
// SetPalMode, which would otherwise rewind it to the texture memory base.
//
// The vifChainFlags zero store and the textureCursorEnd load must each stay a
// standalone self-based absolute pair (two instructions); ps2eeas emits a
// single GPREL16 instead when EGC schedules the access into a jal delay slot,
// which is 16 bytes short. A .data-classified alias plus -mno-split-addresses
// on this TU forces the absolute pseudo. The two trailing GP-relative stores
// (vu1ChainHead, textureCursorEnd) are written before the call whose delay
// slot holds them, because this EGC schedules a store into the delay slot of
// the call it precedes. See decomp_state/notes/draw_ResetVideoPipeline__Fv.md.
void ResetVideoPipeline(void) {
    spaceLoadInProgress = 1;
    resetVideoVifChainFlagsAbs = 0;
    DMAC_VIF1_Disable();
    sceDmaReset(1);
    InitDma();
    sceGsResetGraph(GS_RESET_FULL, GS_INTERLACED,
                    videoModePal ? GS_VIDEO_PAL : GS_VIDEO_NTSC, GS_FIELD_MODE);
    resetVif1Gif();
    VU1_initChain();
    int saved = resetVideoTextureCursorEndAbs;
    vu1ChainHeadStore = 0;
    SetPalMode();
    textureCursorEndGp = saved;
    VU1_initChain();
    DMAC_VIF1_Enable();
}
