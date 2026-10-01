# DrawUIFrame (code/game/draw_post_post.cpp)

- Original: 324 bytes (`0x144`) at vram `0x1F5F18` (file offset `0xF6E98`),
  symbol `DrawUIFrame` (unmangled, C linkage).
- Semantics: draws a beveled UI frame with seven `DrawRectOverlay` calls.
  Color = `alpha << 24 | 0x00040404` (near-black RGB 4/4/4, top byte is the
  caller-supplied alpha, e.g. `(int)(fade * 80.0)`). Calls (top, bot, left,
  right): full rect; then three stepped bevel bands outside each vertical
  edge, vertically inset 1/2/4:
  `(top+1,bot-1,left-2,left)`, `(top+2,bot-2,left-3,left-2)`,
  `(top+4,bot-4,left-4,left-3)`, `(top+1,bot-1,right,right+2)`,
  `(top+2,bot-2,right+2,right+3)`, `(top+4,bot-4,right+3,right+4)`.
- Cross-reference: Deadlocked's `DrawTextShadow(top,bot,left,right,alpha,rgb)`
  (reference/dl/game_dl/draw.cpp:3590) is the same shape with `rgb` a
  parameter instead of the fixed `0x40404`; its `DrawRectOverlay(top,bot,
  left,right,uint64 color)` confirms the (top,bot,left,right) convention.
  DrawRectOverlay's vertex math (a0/a1 -> Y via occlViewParams.minY,
  a2/a3 -> X via minX) confirms it too.
- Callers (all in the unmangled UI/font family): func_001F4BE0
  (DrawSubtitles, blocked), the 0x1FBC50 HUD frame function (0x1FBD14,
  0x1FC2D4, 0x1FC45C), 0x1FE8DC, 0x1FEA28, 0x1FEAC8, 0x1FEB78, 0x1FECA4,
  0x206418.
- Codegen: plain straight-line C matched byte-for-byte on the first probe
  (default flags, no pins). EGC keeps top/bot/left/right/color in
  s1/s2/s3/s6/s0, precomputes top+1 and bot-1 into the 0x0/0x4(sp) slots,
  left-2/left-3/top+2/bot-2 into s4/s5/s7/s8, and re-materializes color into
  a4 before each call. Frame is 0xB0 (9 spilled registers + two ints).
- Lombyte's FUN_001f5f18.c has the same call list (their first call drops
  the arguments; the binary keeps all five).
