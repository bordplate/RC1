# InitViewContext__Fv (0x1F2C60, 308 bytes) — MATCHED

`void InitViewContext(void)` in `code/game/draw_post_post.cpp`.

## Semantics

Initializes the occlusion view context. It has three phases:

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

2. **viewCtx scale defaults.** Writes `field_0A0 = 32.0`, `field_0A4 = 745472.0`,
   `field_0B0 = 0.63` (offsets 0xA0/0xA4/0xB0, newly mapped in camera.h).
   Then the occlusion camera scales (via `func_001FA6C0` = cvt.s.w, int→float):
   - `occlCamScale0 = float(paramX) * 0.5`, `occlCamScale2 = occlCamScale0 * 4.0`
   - `occlCamScale1 = float(paramY) * 0.5`, `occlCamScale3 = occlCamScale1 * 4.0`

3. **viewCtx fog defaults.** `fogFarIntensity = 524288.0`,
   `fogNearIntensity = 255.0`, `fogNearDist = fogFarDist = 0.0`.

## Match-critical details

### The 0.5 multiplier must be a saved float local
`halfScale = 0.5f` stays in `$f20` across the call and both `mul.s`. An inline
`* 0.5f` (or `* 1.0f`/`* 2.0f`) gets folded by EGC into `mov.s`/`add.s`
(`-ffast-math`). Keeping it a local reproduces the original's single `lq $f20`
load and the two `mul.s`.

### paramX reloads from memory, paramY stays a register local
`occlCamScale2` is computed as `viewCtx.occlCamScale0 * 4.0f` — a reload of the
just-stored value from +0x200 (`lqc`), NOT the register holding `paramX*0.5`.
`occlCamScale3` instead uses the register local `yScale` (no reload). This
asymmetry (reload for X, register for Y) is what avoids an extra `$f21` save
and matches the original's store sequence. Symmetrizing (both register, or both
reloaded) changes the size and register allocation.

### The second call reloads paramY signed (fresh `lh`)
call1's arg is the local `x` (`move $a0, v1`). call2's arg is a FRESH signed
16-bit load: `func_001FA6C0(*(const s16*)(paramBase + 0x152))` → `lh $a0, 338(s1)`.
Using the `y` local instead (symmetric) cascades a 46-word register-allocation
diff. The pointer cast is load-bearing; do not "clean it up."

### Trailing zero stores are reversed
The two `0.0f` stores (`fogNearDist` +0x218, `fogFarDist` +0x22C) are emitted by
EGC in REVERSE source order, interleaved around the `occlCamScale1` (`swc1 $f0`)
store. Source order `fogNearDist` then `fogFarDist` yields the original machine
order (`+0x22C` store, then `swc1 $f0`, then `+0x218` store).

### Sign-extension idiom
`(s16)raw` (raw = int holding a u16) compiles identically to the
`(raw << 16) >> 16` idiom in this EGC build (both → `lhu; sll 16; sra 16`); the
cast form is used for readability.

## Layout added to camera.h
- `ViewCtx` fields `field_0A0`/`field_0A4`/`field_0B0` (offset-preserving; the
  old `pad_80[0x10]` was split). Values confirmed 32.0 / 745472.0 / 0.63.
- `struct OcclCamParamBlock` (0x151780) and `struct OcclViewParams` (0x13E500)
  with their `extern "C"` globals (both already in config/symbols.txt).

draw_post_post.o matches all 308 bytes; full boot parity passes.
