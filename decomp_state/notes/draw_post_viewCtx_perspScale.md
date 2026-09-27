# drawCamera == viewCtx.perspScale (viewCtx+0x210) — refactor resolution

Refactor entry `camera_viewCtx_drawCamera_overlap`, resolved 2026-09-27.

## Question
`drawCamera` (declared `extern Camera`, 0x18CF10) and `viewCtx.field_210`
(ViewCtx at 0x18CD00, +0x210) are the SAME address. Is drawCamera a Camera
member embedded in ViewCtx at +0x210, or a separate extern aliasing the region?

## Resolution: separate symbol aliasing viewCtx+0x210, and NOT a Camera
- 0x18CF10 == viewCtx + 0x210 exactly.
- The ONLY C reader of drawCamera was projectWorldPoint (0x1F2070):
  `float scale = drawCamera.mtx0[0] / r4[3];`  (perspective divide).
  Ground-truth load (objdump 0x1F2120/0x1F2128):
    `lui v0,0x19 ; lwc1 $f2,-0x30F0(v0)` -> 0x18CF10. Self-based absolute
    (0x18CF10 is OUTSIDE the gp window gp=0x166C00 +/-32K), so any C form that
    folds to the constant 0x18CF10 emits the identical lui/lwc1.
- The block viewCtx+0x210..0x238 is written PIECEMEAL as individual floats,
  never as a bulk 64-byte matrix copy:
  * UpdateFog (0x1F2588): swc1 viewCtx+0x218/21C/228/22C, sw +0x230/34/38
    (fogNearDist/FarIntensity/NearIntensity/FarDist/R/G/B).
  * FUN_001f2d98 (in the UpdateOcclusion region 0x1F2C10..): `swc1 $f3,528(s0)`
    = viewCtx+0x210 (s0 = viewCtx base 0x18CD00), plus +0x214/0x220/0x224, and
    reads +0x210 back with `lw v0,528(s0)`.
  A camera BASIS matrix (mtx0) cannot simultaneously hold fog params, so
  drawCamera was never a Camera. It is the perspective scale (0x210), the first
  float of the fog+persp param block owned by viewCtx.
- symbols.txt map of the viewCtx region (0x18CD00-0x18CF48): viewCtx 0x18CD00,
  occlCamScale0-3 0x18CF00-0C, drawCamera 0x18CF10, occlColor0-2 0x18CF3C-44.
  The fog struct fields (0x218-0x238) sat inside the drawCamera symbol range.

## Change (parity-preserving)
- camera.h: removed `extern Camera drawCamera;`; renamed ViewCtx `field_210`
  -> `perspScale`; fixed the Camera.mtx0 comment (dropped the stale "perspective
  scale used by projectWorldPoint" attribution, which actually read viewCtx);
  fixed the currentCamera/drawCamera header comment and the field_210 comment.
- draw_post.cpp projectWorldPoint: `drawCamera.mtx0[0]` -> `viewCtx.perspScale`.
- config/symbols.txt + config/linker_aliases.ld: removed the `drawCamera`
  symbol (viewCtx+0x210 is now named via the viewCtx.perspScale field). After
  split, the 0x18CF10-0x18CF3B data region is covered by the occlCamScale3
  label; bytes unchanged (all zero in boot ELF).

## Verification
- `make split && make -j2` clean.
- `tools/tu_assembler_diff.py build/code/game/draw_post.o build/boot_elf.elf`
  -> 4/4 match.
- `cmp build/boot_elf.elf assets/boot_elf.elf` -> byte-identical (checked after
  both the C-only change and the symbol removal).
