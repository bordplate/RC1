#include "common.h"
#include "types.h"
#include "hud.h"

// GetFrameTex (0x1FFA10) reserves 16-byte VU1 data-reference load slots in
// gifLoadSlots (base 0x18D040, slot n at base + n*16, indexed by gifLoadCnt)
// for the palette and/or texture of HUD frame `fi` when either is not yet
// scheduled (gsram == 0), advances textureCursor past each, increments
// gifLoadCnt if a slot was written, and returns the 64-bit GS TEX0 descriptor
// for the frame's texture. See decomp_state/notes for the full decode.
//
// This function is isolated into its own Splat segment / TU assembled by the
// SN assembler (ps2eeas) because the original mixes in-window access modes
// for gifLoadCnt / textureCursor / textureMemoryBase: 2-instruction self-based
// absolute loads (gifLoadCnt everywhere, textureCursor block-2 load,
// textureMemoryBase) plus 1-instruction GPREL16 (textureCursor block-3 load
// and both textureCursor stores, in branch delay slots). A GNU-assembler TU
// (gas) expands a plain named in-window extern as GPREL16 only, so it cannot
// reproduce the self-based absolute form without a forbidden constant-address
// cast. Under SN, plain unseeded in-window externs expand self-based absolute
// for ordinary references and GPREL16 inside EGC's noreorder branch regions,
// which is exactly the original pattern.
//
// BLOCKED (2026-10-09): with the SN split the access modes match, but a
// register-allocation cascade from the prologue keeps 68/136 words off:
// EGC 2.95.2 allocates frames/fr->a1, pals->a2, texs->a0, whereas the original
// uses frames->v0, fr->a0, pals->a1, texs->a2 (the scoped $3 heap pin and the
// $10 wrote pin reproduce the first words and the wrote register but not the
// struct-pointer allocation). Same EGC RA wall family as the blocked sibling
// GIF-paging functions. See decomp_state/notes/hud_post_post2_gftex_GetFrameTex.md.
INCLUDE_ASM("code/_generated/nonmatchings/game/hud_post_post2_gftex", GetFrameTex__Fi);
