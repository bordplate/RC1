# Hud_DrawMsgBar (0x201128, 0xD8 = 216 bytes) — MATCHED 2026-10-12

Source: `code/game/hud_post_post2_post3_tail.cpp`. Splat symbol
`func_00201128` (stripped ELF: no original name recoverable) pinned at
0x201128; the function is the call target of 0x201268 in the HUD message
printer (func_00201200, still INCLUDE_ASM).

## Semantics

`void Hud_DrawMsgBar(int x, int y, int width, int height, int alpha)`
(5 int params, a0-a4). Draws the three-segment icon bar behind the HUD
message text, from HUD bank icon 0x7580 (frame 0 = middle tile, frame 1 =
end cap):

```c
int frameMiddle = GetIconFrame(0x7580, 0);   // -> s5
int frameCap    = GetIconFrame(0x7580, 1);   // -> s4
func_001FFC30(frameCap, x, y, 0x20, height, alpha);                          // left cap
func_001FFC30(frameMiddle, x + 0x20, y, width - 0x40, height, alpha);        // middle
func_001FFE18(frameCap, x + width - 0x20, y, 0x20, height, alpha);           // right cap
```

The final cap goes through the 4-corner builder func_001FFE18 (0x1FFE18)
instead of the two-corner builder func_001FFC30 (0x1FFC30 = Hud_DrawSprite);
both take (frame, x, y, width, height, alpha).

Caller context (verified against the matched Hud_DrawChannels at 0x1FF8C0/
0x1FF8F8 and Lombyte's fun_00201200.c): the printer draws the text shadow
(FontPrintCenterLarge(x+1, y+1, color & 0xFF000000, ...)), then this bar at
(cx-0x20, y-8) with width (x-(cx-0x20))*2 — i.e. centered on the text anchor
x and extending 0x20 beyond each text edge — then the main text. The alpha
arg is the message color's top byte clamped to a minimum of 0x50.

## Match

Straight translation matched on the FIRST probe (tools/decomp_probe.py,
production default flags, no pins/barriers/private flags): 216/216 bytes,
0 word-diffs. No compiler quirk involved — EGC 2.95.2's default scheduler
reproduces the prologue save/move interleaving (sq s1,s0 then the five
`daddu` arg captures in a1,a0,a2,a4,a3 order), the two GetIconFrame calls
with the `addiu a0,0x7580` re-materialized between them, and all three
6-arg call setups (one arg move per call in the jal delay slot: a3 for
calls 1/3, a5 for call 2) byte-for-byte.

Match-sensitive notes:
- Callee prototypes must be `void` (return-ignored; the return type affects
  EGC register allocation per the snd_SendCurrentBatch observation).
- `func_001FFC30`/`func_001FFE18` are called through `extern "C"`
  declarations: their original names are unrecoverable (stripped ELF) and
  the build links the unmangled Splat placeholder names.
- The source name is `Hud_DrawMsgBar` with an `asm("func_00201128")`
  symbol override (declaration + definition form; cfront rejects the
  label between parameter list and `{` on the definition itself).
- Icon id 0x7580 and the 0x20 cap width are named defines
  (HUD_MSG_BAR_ICON / HUD_MSG_BAR_CAP_WIDTH) in the TU.

## Verification

- `tools/decomp_probe.py` standalone: 216/216, 0 diffs.
- Clean `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf`: passes. Splat reclassified func_00201128.s to
  matchings; the TU stays on the GNU-assembler list (the function contains
  no pseudo expansions, so the assembler choice is inert here).
- `tools/decomp_status.py --count`: 524 -> 523.
