# func_001F6CB8 (FontPrintWindowGeneric) — blocked 2026-10-02

Windowed text printer of the FontPrint family (`draw_post_post.cpp:1121`).
`int FontPrintWindowGeneric(int x, int y, int width, int height, long color, u8* text,
int length, int effect, fontLetter* glyphs)` — returns `(finalLineY - y) + 0x10`.
198 words (0x318), frame 0xC0. Blocked after last-resort GPT-5.6 Sol escalation
(21 variants, best candAB 165/198 diffs).

## Verified semantics (all confirmed against original bytes)

- Frame: 16(sp)=x, 20(sp)=y, 24(sp)=width, 28(sp)=bottom; s0-s8 @ 32..160; ra @ 176.
  Register map: s1=color, s2=cursorX (lw from 16(sp)), s3=i, s4=lineY (move from a1),
  s5=length, s6=glyphs (lw from 192(sp) stack arg, in first-beqz delay slot), s7=effect,
  s8=text, s0=p.
- Entry: `fontCtrlColors[0] = (int)color` (64-bit zext: dsll32/dsra32, `sw v0,-13576(a0)`
  with lui a0,0x19) BEFORE the guards.
- Guards, in order: `length==0` -> exit; `*text==0` -> exit; `bottom = lineY + height`
  (in the second beqz DELAY slot), spilled to 28(sp); `bottom < lineY+16` -> exit-B
  (this exit uses a1=y still live; all others do lw a1,20(sp) first).
- Loop `for(;;)`: per iteration reload x (lw 16(sp)) and width (lw 24(sp)), compute
  p = text+i into s0 (addu s0,s8,s3); reset wordW=0 (f1), cursorX->f2; if i==length
  skip measurement; guard c=*p: c==32 skips measurement; c<16 skips measurement (via
  bnezl whose delay slot carries the pen duplicate add.s).
- Measurement (matches byte-for-byte in candidates; the "separated reads" form from the
  expert consultation is required: `u8 glyphChar = *q; j++; q++; wordW +=
  (float)glyphs[glyphChar].advance; if (j==length) break; u8 nextChar = *q; if
  (nextChar==32) break; if (nextChar < 16) break;`) sums advance of chars until
  end/space/control.
- Pen merge: f1 = f2 + f1 (pen = cursorX + wordW); winEnd = x+width -> f0 (mtc1 a2,f0);
  `if ((float)winEnd < pen)` wrap, else draw.
- Wrap: c = *p; **i-- iff c != 0x20 && c >= 0x10** (`movz s3, v0, v1` where v0=i-1,
  v1=(c<16): s3 = (v1==0) ? v0 : s3); cursorX = x (lw 16(sp)); lineY += 16 (in the
  wrap path's `b tail` delay slot).
- Draw: control char iff **c in [8,16)** (fused `addiu v0,c,-8; sltiu v0,8`): if the
  GPREL flag at 0x15F49C (drawOcclusionEnabled) is nonzero, `color &= 0xFF000000`
  (64-bit and) then `color |= (unsigned)fontCtrlColors[c-8]` — palette load is
  `lw` + `and v0,v0,0xFFFFFFFF` (signed lw + zext, NOT lwu), palette base 0x18CAF8.
  Otherwise normal glyph: if glyphs[c].advance == 0 skip; if `((c+128)&0xFF) < 0x28`
  (c in [128,168), sltiu) draw glyphs[c+64] at x = cursorX + glyphs[c].advance, 16x16;
  else if c < 32 draw glyphs[c] 24x16 at cursorX; else if c >= 33 draw 16x16 (c==32
  draws nothing but still advances). Quad args: (x, y=lineY+drop, 24|16, 16, u, v,
  24|16, 16, color, effect).
- Tail: cursorX += glyphs[*p].advance (only on the draw side, inside else); i++;
  break if i==length; break if text[i]==0; continue if bottom (28(sp) reload) >=
  lineY+16 (current lineY).
- Exit: shared: lq ra; subu v0, s4, a1 (lineY - y); lq s8..s0; addiu v0,v0,16; jr ra;
  delay addiu sp,sp,192.

## Key toolchain discoveries (this session)

- **EE register numbering**: on the R5900 `s8` = register **30**; `$24`/`$25` are
  `t8`/`t9` (caller-saved). Pinning `asm("$24")` "for s8" silently puts the value in
  t8 and EGC emits broken code (value used after a clobbering call, no reload).
  Use `asm("$30")`. (Isolated repro: working/FontPrintWindowGeneric/pintest4.cpp.)
- **Pins are soft**: `register T v asm("$N")` is respected only when uncontested.
  Pinning all nine s-regs (s0-s8) while a 10th long-lived value (y) needs a home
  makes EGC assign y's home to s8 (text's reg) and silently miscompile.
- The expert (GPT-6 Astra) consultation solved the inner measurement loop (the
  `bnezl` back-edge + delay-slot c-load + in-place v0 head) via the separated-reads
  loop form.
- Last-resort (GPT-5.6 Sol) tested 21 more variants (candL..candAC): volatile a-arg
  homes, plain bottom (natural 28(sp) spill), per-iteration `register int winEnd
  asm("$6") = xIter + widthIter;` (keeps the f0/f1/f2 float map), tied palette
  pointer (adds unwanted `addiu a0,v1,%lo`), p/text/palette barriers, both scheduler
  flags. Best candAB: 756B, 165 diffs, no match.

## The wall

A coupled pre-RA scheduling/allocation decision EGC 2.95.2 will not make for this
RTL: (1) p becomes a running induction pointer (initial `move s0,s8` + tail update)
instead of per-head `addu s0,s8,s3`; (2) the pinned winEnd is computed before the
length branch and `addu a2,v0,v1` will not land in the `beq s3,s5` delay slot;
(3) the palette base cannot be a0 in the symbolic lui/%lo-store form; (4) the
a-arg save/reload-from-stack pattern (y@20(sp), per-iter width reload, exit
lw a1,20(sp)) is not reproduced. Barriers and scheduler flags each break a
different required property (e.g. -fno-schedule-insns -> f1/f2/f3 float map).

## Artifacts

- Probes: working/FontPrintWindowGeneric/{candD,candE,candF,candG,candH,candI,
  candJ,candK,candL..candZ,candAA,candAB,candAC}.cpp; best result ABv/candidate.json.
- Original ground truth: working/research_001F6CB8/orig_001F6CB8.asm (+ REPORT.md).
- Dossier for the escalations: lastresort_dossier.md (+ expert_dossier.md).

## Follow-up (queue)

func_001F6FD0 (FontPrintWindowSmall, 25 words) is the small-medium wrapper:
7-arg void `(x,y,width,height,color,text,length)` ->
`FontPrintWindowGeneric(..., GetEffectTex(FONT_EFFECT_TEX_MEDIUM),
(fontLetter*)fontMediumGlyphs)`; frame 0x90. func_001F7070 (0x1C) is a dead tail
fragment (verified 0 refs) to emit as inline assembly per the EGC dead-tail rule.
