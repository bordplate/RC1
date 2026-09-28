# ResetDrawGlobals (0x1F37E8, 0x7C = 31 words)

Per-level draw-state reset in `code/game/draw_post_post.cpp`. A 31-word flat
body of 17 zero-stores (no frame, no calls, no saves) in a fixed,
non-address-ordered sequence; the 17th store (`mobyOcclClusterCount`) lands in
the `jr $ra` delay slot. Called from the level loader (`func_001E9B10`) and
the space loader (`func_00230F60`). `extern "C"` (callers use the unmangled
symbol). Matched byte-for-byte on the first SN candidate.

## Store order (the codegen contract)
EGC emits the independent stores in exact source statement order (verified:
0 word diffs). The order is NOT address-ordered (e.g. drawCallback2Count
0x15F468 is stored 4th, after 0x15F46C/0x15F470):

| # | addr | global | mode |
|---|------|--------|------|
| 1 | 0x15F464 | drawCallbackCount | self |
| 2 | 0x15F46C | drawCallback3Count | self |
| 3 | 0x15F470 | drawCallback4Count | self |
| 4 | 0x15F468 | drawCallback2Count | self |
| 5 | 0x15F474 | effectQuadCount | self |
| 6 | 0x15F330 | D_0015F330 (unknown) | self |
| 7 | 0x15F334 | D_0015F334 (unknown) | self |
| 8 | 0x15F444 | occlChainActive | self |
| 9 | 0x15F448 | occlChainFrames | self |
| 10 | 0x15F34C | screenOverlayEnabled | **GPREL** |
| 11 | 0x15F360 | D_0015F360 (unknown) | **GPREL** |
| 12 | 0x15F370 | occlDebugOverlayEnabled | **GPREL** |
| 13 | 0x15F648 | OcclMode | self |
| 14 | 0x161190 | occlSampleBase | self |
| 15 | 0x161194 | occlCellTable | self |
| 16 | 0x161198 | occlCellCount | self |
| 17 | 0x16119C | mobyOcclClusterCount | **GPREL** (jr delay slot) |

## Access modes
- **Self-based (13):** plain `extern int` → EGC emits a bare pseudo and
  ps2eeas expands it self-based (`lui $1,%hi; sw $0,%lo($1)`) because EGC's
  own `.extern` lands after the reference. (Same mechanism as the SN
  `drawCallbackCount` match in AddDrawCallback and `spaceLoadInProgress` in
  drawNormalFrame.)
- **GPREL (4):** `.extern`-seeded declarations
  (`asm(".extern <name>, 4");` before the function) make ps2eeas expand the
  bare pseudo as GPREL16 (`sw off gp`) — the `partClipDistGp` pattern.

## Global meanings (researched via decomp-researcher)
- drawCallback3/4Count: counts for the "pre effects" / "vu effects" callback
  lists (funcs 0x18DF40/0x18E140, args 0x18E040/0x18E240), run by
  func_001F46C8 / func_001F4740.
- effectQuadCount: count of 16-byte-stride effect quads (0x18E350) drawn by
  func_001F4880.
- occlChainActive/Frames: occlusion-sample VU1 chain state machine
  (func_001F4D98): enable pulse + a frame counter ramping to 24 and decaying,
  emitting a per-frame 0x40-byte VU1 chain.
- screenOverlayEnabled: gates the "screen overlay" stage (func_001F4FB8).
- occlDebugOverlayEnabled: gates func_001F5138 (occlusion-debug GS-register +
  rect overlay).
- occlSampleBase/CellTable/CellCount/mobyOcclClusterCount: per-cell
  occlusion-sample table (vendor/func_00239D60 search + func_00239F58
  bilinear sampler) and the moby occlusion z-test cluster count.
- D_0015F330/334/360: debug/profiler/overlay state, write-only in the boot
  ELF (no readers) — address-based names retained.

## For the sibling functions (not part of this match)
Several of these globals are accessed in MIXED mode elsewhere (e.g.
occlChainFrames read+written both GPREL and self-based in func_001F4D98;
occlCellCount both modes in vendor/func_00239D60; OcclMode written GPREL in
freeze/UpdateModeFreeze__Fv). When those functions are decompiled they each
need the snd_BankLoadByLoc two-alias (per-access-site) treatment.
