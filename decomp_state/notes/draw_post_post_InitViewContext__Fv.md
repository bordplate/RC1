# InitViewContext__Fv (0x1F2C60, 308 bytes) — MATCHED

`void InitViewContext(void)` in `code/game/draw_post_post.cpp`.

## Semantics

Initializes the view context. It has three phases:

1. **View rectangle (occlViewParams, 0x13E500).** Reads the two 16-bit
   params from `occlCamParamBase` (0x151780, u16 fields at +0x150 and +0x152).
   Each is sign-extended (`(s16)raw` = `lhu; sll 16; sra 16`) and a halved
   copy is kept (`>> 1` = `sra 17`). Writes eight s32 fields:
   - `paramX/paramY` = sign-extended params
   - `halfX/halfY` = params >> 1
   - `minX = (OCCL_VIEW_CENTER - halfX) << 4`, `maxX = (halfX + OCCL_VIEW_CENTER) << 4`
     (and the Y pair). `OCCL_VIEW_CENTER = 0x800`; the `<< 4` scales into 16.0
     fixed-point. So min/max form a rectangle centered on `0x800 << 4` with
     half-extent `halfX/halfY << 4`.

2. **viewCtx projection defaults.** Writes `nearClip = 32.0` (0xA0),
   `farClip = 745472.0` (0xA4) and `xratio = 0.63` (0xB0) as named constants
   (`VIEWCTX_NEAR_CLIP`, `VIEWCTX_FAR_CLIP`, `VIEWCTX_DEFAULT_XRATIO`).
   Then the pixel/clip values (via `func_001FA6C0` = cvt.s.w, int→float):
   - `xpix = float(paramX) * 0.5`, `xclip = xpix * 4.0`
   - `ypix = float(paramY) * 0.5`, `yclip = ypix * 4.0`

3. **viewCtx fog defaults.** `fogFarDist = 524288.0`,
   `fogNearIntensity = 255.0`, `fogNearDist = fogFarIntensity = 0.0`
   (`VIEWCTX_DEFAULT_FOG_FAR_DIST`, `VIEWCTX_DEFAULT_FOG_NEAR_INTENSITY`).

## Field semantics (established 2026-09-27)

The formerly magic literals and pad fields were resolved from the consumer,
`UpdateViewContext__Fv` (0x1F2D98, still INCLUDE_ASM), which matches the
descendant reference `reference/dl/game_dl/draw.cpp` UpdateViewContext line
for line:

- `nearClip`/`farClip` (0xA0/0xA4, `VC.D`/`VC.F` in DL): the perspective-box
  depths. fMtx uses `xpix/(xratio*nearClip)` / `ypix/(yratio*nearClip)` for
  the x/y scales and `(farClip+nearClip)/(farClip-nearClip) * -2^23` and
  `(-2*farClip/(farClip-nearClip)) * -2^23` for the depth rows.
- `xratio` (0xB0): default aspect ratio; the DL default is 0.62 and callers of
  `SetUserViewContext` (0x1F33B8, INCLUDE_ASM) override it per level.
  `yratio` (0xB4) = `xratio * 0.775` when `videoModePal == 0` (NTSC) else
  `xratio * 0.756` (PAL).
- `xclipratio`/`yclipratio` (0xA8/0xAC) = `xclip/xpix`, `yclip/ypix`, set by
  UpdateViewContext.
- `xradpad`/`yratio`-derived (0xB8/0xBC) = `1/cos(atan2(1, xratio))` /
  `1/cos(atan2(1, yratio))`.
- `xpix`/`ypix`/`xclip`/`yclip` (0x200-0x20C): half draw width/height in
  pixels and 4x clipbox extents; SetUserViewContext stores
  `width * 0.5` / `height * 0.5` / `* 4.0`.

### Fog far-pair swap

The 0x21C/0x22C fields were misnamed before this refactor: 0x21C is
`fogFarDist` and 0x22C is `fogFarIntensity` (not the other way round). Three
independent proofs:

1. **Fog formulas** in UpdateViewContext:
   `fogMult = (fogFarIntensity - fogNearIntensity) / ((fogFarDist - fogNearDist) / 1024)`
   uses 0x21C-0x218 as the distance difference and 0x22C-0x228 as the
   intensity difference — identical to the DL, which names them that way.
   `fogAdd = fogNearIntensity - fogNearDist/1024 * fogMult`,
   `fog1 = (fogNearIntensity*fogFarDist - fogFarIntensity*fogNearDist)/(fogFarDist-fogNearDist)`.
   Distances are stored in 1/1024 units; intensities are 0-255 with 255 = clear.
2. **Level-fog writer** `Camera_UpdateFog` (0x1EE4B0, INCLUDE_ASM): scales the
   volume's far-distance component by 1024 and stores it to 0x15F48C, and
   computes `255 - x * 255` for the far intensity stored to 0x15F494.
3. **Clamp code** (0x1F17BC / 0x1F1908): clamps 0x15F48C to
   <= 524288.0 (= the boot default `fogFarDist`) with a 1024.0 tolerance and
   enforces `fogNearDist <= fogFarDist`.

So the globals swapped to match: `levelFogFarDist = 0x15F48C`,
`levelFogFarIntensity = 0x15F494`, `waterFogFarDist = 0x1610CC`,
`waterFogFarIntensity = 0x1610D4` (same block layout; waterFog is
overlay-written, never in the boot ELF). The `UpdateFog__Fi` local stores and
the `ViewCtx` fields were renamed accordingly. Boot fog defaults:
`fogFarDist = 524288.0` (= 512 distance units) and
`fogNearIntensity = 255.0` (clear) with the far end fully fogged (0).

## Match-critical details

### The 0.5 multiplier must be a saved float local
`halfScale = VIEWCTX_HALF_PIXEL` (0.5f) stays in `$f20` across the call and
both `mul.s`. An inline `* 0.5f` (or `* 1.0f`/`* 2.0f`) gets folded by EGC
into `mov.s`/`add.s` (`-ffast-math`). Keeping it a local reproduces the
original's single `lq $f20` load and the two `mul.s`.

### The 4.0 must stay an inline constant
`xclip = xpix * 4.0f` / `yclip = ypix * 4.0f`: the original has no separate
`mov.s` for the 4.0, so it stays an inline literal via `VIEWCTX_CLIP_SCALE`
(do not make it a local or re-express it).

### paramX reloads from memory, paramY stays a register local
`xclip` is computed as `viewCtx.xpix * 4.0f` — a reload of the just-stored
value from +0x200 (`lqc`), NOT the register holding `paramX*0.5`. `yclip`
instead uses the register local `yScale` (no reload). This asymmetry (reload
for X, register for Y) is what avoids an extra `$f21` save and matches the
original's store sequence. Symmetrizing (both register, or both reloaded)
changes the size and register allocation.

### The second call reloads paramY signed (fresh `lh`)
call1's arg is the local `x` (`move $a0, v1`). call2's arg is a FRESH signed
16-bit load: `func_001FA6C0(*(const s16*)(paramBase + 0x152))` → `lh $a0, 338(s1)`.
Using the `y` local instead (symmetric) cascades a 46-word register-allocation
diff. The pointer cast is load-bearing; do not "clean it up."

### Trailing zero stores are reversed
The two `0.0f` stores (`fogNearDist` +0x218, `fogFarIntensity` +0x22C) are
emitted by EGC in REVERSE source order, interleaved around the `ypix`
(`swc1 $f0`) store. Source order `fogNearDist` then `fogFarIntensity` yields
the original machine order (`+0x22C` store, then `swc1 $f0`, then `+0x218`
store).

### Sign-extension idiom
`(s16)raw` (raw = int holding a u16) compiles identically to the
`(raw << 16) >> 16` idiom in this EGC build (both → `lhu; sll 16; sra 16`); the
cast form is used for readability.

## Layout added to camera.h
- `ViewCtx` fields `nearClip`/`farClip` (0xA0/0xA4), `xratio` (0xB0),
  `xpix`/`ypix`/`xclip`/`yclip` (0x200-0x20C), `fog1` (0x214),
  `fogMult`/`fogAdd` (0x220/0x224) and the corrected fog far pair
  `fogFarDist` (0x21C) / `fogFarIntensity` (0x22C).
- `struct OcclCamParamBlock` (0x151780) and `struct OcclViewParams` (0x13E500)
  with their `extern "C"` globals (both already in config/symbols.txt).

draw_post_post.o matches all 308 bytes; full boot parity passes (re-verified
after the 2026-09-27 rename/refactor).
