# drawDebugFont (0x1F79A8, matched 2026-10-03)

Per-frame debug font stage. Called only from `DrawDebugProfiler` (0x1F3BE4)
for the `drawEnableMask & 0x20` stage bit, between `SetupGifPaging(1)` and
`DoGifPaging()`. When `fontQuadCount` (0x15F478, filled by level overlays —
no boot-ELF writes exist) is non-zero, the function:

1. programs four font GS registers: `VU1_addGSregister` 8/5, 0x14/0x61,
   0x47/0x513F1, 0x4A/1;
2. calls `prepareFontQuads` (0x1F8FF0);
3. sets `fontDepthBias = -0.04f` (`DEBUG_FONT_DEPTH_BIAS`);
4. queues the font VU state via `FontQueueVUState`;
5. calls `drawFontQuads` (0x1F89A4);
6. resets the bias with an **int** zero store (`sw $0`, hence
   `*(int*)&fontDepthBiasGp = 0`) and `VU1_addGSregister(0x4A, 0)`.

The GS register numbers (8, 0x14, 0x47, 0x4A) exceed the commonly documented
GS register file and in-game usage reaches 0x200, so the hardware semantics
could not be confirmed from any available reference (2026-10-03); they are
kept as literals in the source with a comment. The -0.04f bias is named
`DEBUG_FONT_DEPTH_BIAS` (it is added to the font state matrix rows in
`FontQueueVUState` and zeroed again after the draw).

## Pipeline data (block 0x15F478-0x15F483, shared with drawquad.s)

- `fontQuadCount` (0x15F478): int, number of glyph records (data is
  `.float 0` in the boot ELF).
- `fontQuadItems` (0x15F47C): pointer to 0x20-byte glyph records:
  +0x00 vec4 anchor vertex, +0x0C float (subtracted in the sphere radius),
  +0x10 float depth reference, +0x14 computed u32 alpha, +0x18 computed
  64-bit texture-cursor descriptor.
- `fontQuadIndices` (0x15F480): pointer to an array of u32; each element is a
  pointer to a per-glyph descriptor {u32 w @0, u32 h @4, ...} whose polygon
  vertices start at +8 (0xC stride) with the vertex count in *ptr >> 16.

## prepareFontQuads (0x1F8FF0, drawquad.s)

- SIF0 DMA setup through the 0x1000D400 register block: MADR=0x19C1C0 (VU0
  local memory), SADR=0x2400 (IOP), QWC=0x10 (64 bytes), CHCR=0x100 (start).
- `textureCursor` += 0x400, then per glyph += (1 << (w + h)).
- Per glyph (items stride 0x20, indices stride 4): `FastBSphereCheck` with
  radius (depthRef + 16.0f) - c; outside (code < 0) skips the glyph; camera-
  space transform via `currentCamera.mtx0` rows @0x0/0x10/0x20 minus
  `currentCamera.pos` @0x140 (parallel, w zeroed); alpha: transformed x <=
  depthRef -> s6 | 0xFF (s6 is caller-provided, callee-saved, and not written
  by anything in the boot ELF — meaning not established), else
  (int)(127.0f - (x - depthRef) * 7.9375f) (slope 127/16); sphere code == 0
  (fully inside) additionally sets bit 0x80000000.
- Writes the item's 64-bit descriptor at +0x18: (newCursor>>8) |
  ((1 << max(0, w-6)) << 14) | (0x13 << 20) | (w << 26) | (h << 30) | (1 <<
  34) | ((oldCursor>>8) << 37) | (1 << 62).
- Appends a 16-byte GifLoad slot (see draw_post_GetEffectTex__Fii.md) to the
  ring at D_0018D040 while `gifLoadCnt` < 0x40: [0]=bitSwapLut (0x18E740),
  [4]=0, [6]=oldCursor>>8, [8]=descriptor+0x10, [12]=w, [13]=h,
  [14]=newCursor>>8.

## drawFontQuads (0x1F89A4, drawquad.s)

GIF packet writer for the glyph quads into the VU1 chain (vu1ChainHead):
spins on the GS register window (0x1DFF80-0x1DFFAC), then per glyph with
alpha != 0 emits the textured polygon packets using the item alpha (+0x14),
the 64-bit descriptor (+0x18), and the per-glyph descriptor's vertex data
(t1+8, 0xC stride, count *t1>>16), with per-vertex packed
0xAARRGGBB words (RGB 0xFFFFFF, A = item alpha).

## Match notes

Matched with default flags: `fontQuadCount` must be `int` (a `float` decl
emits `l.s`+`c.eq.s`+`bc1t` instead of `lw`+`beqz`), and the bias reset must
be an int store (a `fontDepthBiasGp = 0` float store emits `swc1` of 0.0f,
not the original `sw $0`). decomp_probe.py: 136/136, 0 diffs; full boot ELF
cmp passes.

## Naming (resolved 2026-10-03, refactor draw_post_post_func_001F79A8)

- `func_001F79A8` -> `drawDebugFont`
- `func_001F8FF0` -> `prepareFontQuads`
- `func_001F89A4` -> `drawFontQuads`
- `D_0015F478` -> `fontQuadCount`, `D_0015F47C` -> `fontQuadItems`,
  `D_0015F480` -> `fontQuadIndices`
- -0.04f bias -> `DEBUG_FONT_DEPTH_BIAS`

Names added to config/symbols.txt (drawquad section) so the generated
drawquad.s glabels, the data lit, and the DrawDebugProfiler jal all use the
real names; no codegen changes — full boot ELF cmp passes. The GS register
numbers stay literal (semantics unconfirmed, see above). The separate
FontPrint/DrawTexturedQuad family (fontCtrlColors 0x15F4A0 / 0x18CAF8) is a
different text path and is not part of this pipeline. func_001F92B0
(0x398, also in drawquad.s) is a sibling debug screen-quad pass using the
0x18ED00 table; still unrenamed (out of scope for this refactor).
