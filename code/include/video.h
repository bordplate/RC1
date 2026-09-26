#ifndef VIDEO_H
#define VIDEO_H

#include "types.h"

// GS (Graphics Synthesizer) reset parameters for sceGsResetGraph.
#define GS_RESET_FULL 0
#define GS_INTERLACED 1
#define GS_VIDEO_NTSC 2
#define GS_VIDEO_PAL 3
#define GS_FIELD_MODE 0

// C linkage: SDK routine resets VIF1, VU1 and GIF hardware.
extern "C" void resetVif1Gif(void);
// C linkage: SDK GS reset routine, mode/interlace/video-system/field-mode.
extern "C" void sceGsResetGraph(short mode, unsigned short interlace,
                                unsigned short videoSystem, unsigned short fieldMode);
// C linkage: SDK DMA reset (libdma.c). Zeroes the enabled channels' control
// registers, re-masks DMAC_STAT, and re-enables DMAC_CTRL when enable is 1.
extern "C" int sceDmaReset(int enable);
// C linkage: handwritten DMA initializer in the game/miscproc assembly region.
extern "C" void InitDma(void);

void DMAC_VIF1_Enable(void);
void DMAC_VIF1_Disable(void);
void VU1_initChain(void);

// The original SetPalMode__Fi consumes no argument, and its callers supply
// none. Keep the historical symbol without manufacturing an argument load.
void SetPalMode(void) asm("SetPalMode__Fi");

// In-window scalar: a plain extern (no .data) keeps the -G8 small-data bare
// pseudo that ps2eeas expands to the original's self-based absolute load
// (see bloaders_LoadDebugFont.md). InitOnce reads the disc region; 0 selects
// GS NTSC (2), 1 selects PAL (3).
extern u32 videoModePal;

#endif
