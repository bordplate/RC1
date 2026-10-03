# func_001F79A8 (matched 2026-10-03)

`setupFontVUState`-like helper: when the shared int flag `D_0015F478` is
non-zero, programs four font GS registers (`VU1_addGSregister` 8/5, 0x14/0x61,
0x47/0x513F1, 0x4A/1), calls `func_001F8FF0`, sets `fontDepthBias = -0.04f`
(0xBD23D70A), queues the font VU state via `FontQueueVUState`, calls
`func_001F89A4`, resets the bias with an **int** zero store (`sw $0`, hence
`*(int*)&fontDepthBiasGp = 0`), then `VU1_addGSregister(0x4A, 0)`.

- Called only from `DrawDebugProfiler` (0x1F3BE4), inside the
  `drawEnableMask & 0x20` GIF-paging path between `SetupGifPaging(1)` and
  `DoGifPaging()`.
- `D_0015F478` is a 4-byte value (data shows `.float 0`) read as an **int**
  (`lw`+`beqz`) here and in the quad drawer (drawquad.s 0x1F8A6C/0x1F9048);
  the block D_0015F478/D_0015F47C/D_0015F480 is shared with drawquad.
- `func_001F8FF0` / `func_001F89A4` are C-linkage functions defined in
  `code/_generated/game/drawquad.s` (matched file), referenced only by address
  (ELF is stripped).

Matched with default flags: `D_0015F478` must be `int` (a `float` decl emits
`l.s`+`c.eq.s`+`bc1t` instead of `lw`+`beqz`), and the bias reset must be an
int store (a `fontDepthBiasGp = 0` float store emits `swc1` of 0.0f, not the
original `sw $0`). decomp_probe.py: 136/136, 0 diffs; full boot ELF cmp passes.

## Remaining naming (open refactor)
- `func_001F79A8` -> real name (best guess `setupFontVUState` /
  `prepareFontDraw`).
- `func_001F8FF0`, `func_001F89A4` (drawquad.s) -> real names.
- `D_0015F478` -> real name (shared int flag).
- GS register numbers (8, 0x14, 0x47, 0x4A) and the -0.04f bias -> named
  constants once their GS semantics are confirmed.
