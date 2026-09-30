# func_001F4BE0 (DrawSubtitles, 0x1F4BE0, 0x1B8) — BLOCKED 2026-09-30

`DrawSubtitles` (draw_post_post.o, default flags, Splat segment
`[0xf31e0, cpp, game/draw_post_post]`). Iterates the null-terminated list of
16-byte subtitle entries for the current `Scene.iframe`; for the first entry
whose `[start,stop)` window contains `Scene.iframe` it draws a font window and
returns. Early-returns if `Scene.subtitles == 0` or `sub->start < 0`. Frame
0x70, callee-saved s0/s1/s2 = text ptr / &occlViewParams / Scene base.

## Verified layout

- `Subtitle` (16 B): `s16 start(+0)`, `s16 stop(+2)`, `s16 textOfs[5](+4..0xD)`,
  `s16 pad_0E(+0xE)`. Entry stride 16; the per-entry text offset advances by 8
  shorts (16 B) per step.
- `SceneDef` @ 0x18CB20 (Splat `D_0018CB20`): `int iframe(+0x34)`,
  `Subtitle* subtitles(+0x4C)`.
- `OcclViewParams` @ 0x13E500: `s32 paramX(+0)`, `s32 paramY(+4)`.
- `FontWindow` (12 shorts): x,y,w,h,textX,textY,maxTextH,totalH,lineH,flags,offX,offY.
- `gameLanguage` @ 0x15ED88 (int).
- Callers' callee: `FontSetWindow` (body INCLUDE_ASM, stores each arg with `sh`),
  `func_001F7580` (body INCLUDE_ASM — draw a subtitle string into the window),
  `DrawUIFrame` (body INCLUDE_ASM).

## KEY confirmed finding — the color argument is 64-bit

`func_001F7580`'s color formal is a **64-bit** value (`unsigned long` on the
EE), **not** `u32`. Probe-verified with the exact project flags:
- `u32` formal → `lui r,0x80b0; ori r,r,0xb0b0` (2 insns, compiler emits `li` pseudo).
- `unsigned long` formal → `ori r,$zero,0x80b0; dsll r,r,16; ori r,r,0xb0b0`
  (3 insns, compiler emits `dli` pseudo) — **EXACTLY the original's form at
  both call sites**.

Changing the prototype to `unsigned long rgba` cut the word-diff count from 78
to 50 and aligned both functions to the same length. This is real and confirmed;
it is NOT committed because the function still does not match (50 residual
diffs). **Carry-forward: when func_001F7580 is decompiled, its color param is
`unsigned long`.**

## Best verified candidate (p9, default flags + 64-bit color) = 50 diff words

All 50 residuals are EGC 2.95.2 Reload/scheduler tie-breaks, NO logic errors
(the candidate is semantically correct and structurally identical):

1. Scene-base temp register: orig `t0` vs cand `a3` — cascades to the subtitles
   load (`lw a0,76(t0)` vs `lw v0,76(a3)`), the `beqz`, the `move`, and the
   start sign-extend (`sll v0,a3` vs `sll v0,a0`) — ~5 diffs.
2. Start-cache `lhu` position: orig at prologue slot #14 (between `addiu a0,v1,-2`
   and `sltu`), cand at #17 (after `movn`) — a 4-line swap — ~4 diffs.
3. Text-pointer association: orig `(sub+4)+langIdx*2` (`addiu a0,a2,4; addu
   s0,a0,v0`), cand `sub+(4+langIdx*2)` (`addiu v0,v0,4; addu s0,a2,v0`) — ~5.
4. `FontSetWindow` arg-constant order: orig `addiu t2,t2,-56; li a2,520; li
   t0,472; li t1,256`, cand permutes these and uses different regs — ~4.
5. edge/textY block (`edge=(s16)((u16)op->paramY-0x3C); win.textY=edge; if
   (op->paramY-0x14 < edge+halfT) win.textY=(u16)op->paramY-(((s16)win.totalH
   >>1)+0x19);`): register differences (edge in t2 vs t1, several sll/sra/addu
   reorderings) — ~20. LARGEST chunk.
6. Tail while-check + epilogue: orig backedge `lh v0,0(a2); bgez v0; <delay
   lhu a3,0(a2)>` (two separate loads), cand `lhu v0,0(a2); move a0,v0; sll
   v1,a0,16; bgez v1` (one load + sign-extend) — ~10.

## Attempted (all in fast isolated probes, exact project flags; results)

- ~10 source forms (text-pointer expressions, `start` placement before/after the
  `<0` pre-check, signed/unsigned start locals): no effect on RA.
- Zero-byte tied asm barriers on `start`, the text base, and the Scene base:
  each restructured the prologue and regressed badly (51 → 101 → 112 diffs).
- Scheduler flags on p9: `-fno-schedule-insns` → 97 (worse);
  `-fno-schedule-insns2` → 53 (worse); default → 50 (best).
- Scene-base hard pin `register SceneDef* init asm("$8")=&Scene` + tied transfer
  to a callee-saved local (startlevel technique): 112 (much worse; reordered
  the sq prologue).
- Color width: u32 → 78; `unsigned long` → 50 (the one win).
- last-resort loop-top `start = e->start` (p14): 99 diffs + function shrank to
  107 lines (loop restructured) — regressed.
- last-resort `int edge` (p15, on p9): 69 diffs — regressed.

## Conclusion

ECG 2.95.2's default Reload/scheduler tie-breaks for the Scene-base-in-`t0`
choice and the edge/textY block cannot be reproduced by any source form,
register pin, zero-byte barrier, or tested TU flag without regressing another
cluster. Blocker recorded; source left as the generated `INCLUDE_ASM`
(parity preserved). The 50-diff best candidate = the structure described
above with `func_001F7580`'s color param declared `unsigned long` (probe
p9, now cleared from the working directory; reconstruct from this note +
the generated asm).
