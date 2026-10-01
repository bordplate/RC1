# FontPrint (0x1F62B0, 0x280) — blocked

`code/game/draw_post_post.cpp:969` (INCLUDE_ASM), part of a 7-member font-print
family (FontPrint/Large/Small/Center/CenterSmall/CenterLarge/Window). Prints a
glyph-string; each char either updates the color (control char) or draws a
glyph quad via `DrawTexturedQuad`.

## Verified semantics
- Signature (from callee prologue `ld s1,0x20(sp)`/`ld s0,0x28(sp)`):
  `extern "C" void FontPrint(int x, int y, unsigned long color, u8* text,
  int length, int tex, fontLetter* glyphs)`. 7 args.
- `fontLetter = struct { u8 u; u8 v; s8 drop_v; s8 advance; }` (0/1 `lbu`,
  2/3 `lb`).
- Calls `extern "C" void DrawTexturedQuad(int x,int y,int w,int h,int u,int v,
  int texW,int texH,long color,long tex)` (10 args; arg9 color→`0(sp)`,
  arg10 tex→`8(sp)`).
- Globals: `fontCtrlColorsInstalled` 0x15F4A0 (plain extern, self-based
  `lui/lw`), `fontCtrlColors[8]` 0x18CAF8 (plain extern, absolute),
  `drawOcclusionEnabled` 0x15F49C (GPREL, plain extern).
- tint: `int tint = avg + ((int)(color & 0xFF000000) + (avg << 16) +
  (avg << 8));` where `avg = (int)((color&0xFF) + ((color>>8)&0xFF) +
  ((color>>16)&0xFF)) / 3` with `div` + `beqzl s7; break 0,7` guard.
- Control char (c-8 < 8): when occlusion enabled, `color = (color &
  0xFF000000) | (fontCtrlColors[c-8] & 0xFFFFFF)` — in-place two-statement
  `and s1,s1,v1` / `or s1,s1,v1`.
- Non-control (glyphs[c].advance != 0): if `((c+0x80)&0xff) < 0x28` draw
  extended glyph `c+0x40` at `(x+advance, y+drop_v)` size 0x10 color; if
  `c < 0x20` draw size 0x18 at `(x, y+drop_v)` tint; else if `c >= 0x21` draw
  normal size 0x10 at `(x, y+drop_v)` color; then `x += advance`.

## Loop structure (original)
```
head(31): lbu v0,0(s0)      # c = *p  into v0
          nop
          addiu v0,v0,-8    # c-8 IN PLACE (destroys c)
          sltiu v0,v0,8
          beqz v0           # if (c-8) < 8 -> control
control:  lw occl (GPREL); beqz end
          [mask 0xff000000 build]
          lbu v0,0(s0)      # c reload IN v0 (41)
          and s1,s1,v1      # color &= 0xff000000 (in place)
          addiu v0,v0,-8    # c-8 IN PLACE (43)
          [fontCtrlColors[c-8] base+index; palette & 0xffffff]
          or s1,s1,v1       # color |= palette (in place)
nonctl:   lbu a0,0(s0)      # c reload IN a0 (53)
          sll v0,a0,2       # c*4
          ...
tail:     addiu s8,s8,1     # i++
          beq s8,s6         # if i==length break
          addiu s0,s0,1     # p++
          lbu v0,0(s0)      # c = *p (146, test)
          bnezl v0,head     # (147) branch-LIKELY
          lbu v0,0(s0)      # (148) DELAY SLOT = next iter's head c load
epilogue: lq...; jr ra; addiu sp,sp,176   (NO trailing nop)
```

## The blocker: a fundamental EGC 2.95.2 scheduling trade-off
The head's c is loaded into **v0** and c-8 is computed **in place**
(`addiu v0,v0,-8`), which DESTROYS c and forces fresh reloads in the
sub-paths. The back-edge is **`bnezl`** with the next-iter head c-load in its
**delay slot** (word 148). These two properties are mutually exclusive under
EGC 2.95.2 for this function:

- **Clean form** (`do { ...; p++; } while (*p != 0);`, no local c, no
  barrier) gives the **correct `bnezl` back-edge + c-load in the delay slot**
  but the head's c lands in **a0** and c-8 is **fresh** (`addiu v1,a0,-8`),
  so EGC CSEs the head's a0 into the sub-paths (wrong head + ~4 extra CSE
  words). [experiment20.cpp, 128 diffs]
- **Tied-barrier form** (the last-resort recommendation: `unsigned int c = *p;
  asm volatile("" : "+r"(c)); c -= 8u;`) gives the **exact original head**
  (`lbu v0; nop; addiu v0,v0,-8; sltiu v0,v0,8; beqz v0`) **and** the in-place
  control/non-control c-reloads, but it **breaks the loop edge**: the back-edge
  becomes **`bnez`** (not `bnezl`) and the delay slot becomes a **nop** (the
  next-iter head c-load is no longer schedulable there). [experiment24.cpp =
  N14, 45 real linked diffs — the best result; experiment21/23 = N11/N13
  intermediate]
- **Hard register pin** (`register ... asm("$2")`) also gives the v0 head but
  breaks the back-edge identically (bnez + nop). [experiment14/18 = N4/N8]

The tied barrier is a scheduling barrier at the loop head; it fixes the
in-place head but prevents EGC from placing the next-iter head c-load in the
`bnezl` delay slot, so the branch demotes to `bnez`. No clean source form,
register pin, barrier, or tested flag yields BOTH the in-place v0 head AND the
`bnezl`+delay-c-load back-edge.

The matched sibling `fontMeasureString` (0x1F6200, lines 914-949) uses the same
tied-barrier idiom (`asm volatile("" : "+r"(c) : "r"(p))`) and matches — but its
back-edge is `bnez` (with `i++` in the delay slot), NOT `bnezl`. FontPrint's
`bnezl`+c-load-delay-slot back-edge is the distinguishing feature the barrier
cannot preserve. FontPrint also has higher register pressure (9 live saved
values vs fontMeasureString's 6), so a local c is pushed to a saved reg
(experiment19/17 = N9/N7, worse).

## Residuals on the best candidate (N14, 45 real diffs)
Beyond the back-edge/nop (words 147-148): the control path still loads the
c-reload into a0 then `move` to v0 (original loads directly into v0) and its
load order (c-reload before the occlusion load) differs; the non-control
CSE-folds `(c+0x40)*4` into one `addiu v0,a1,256` where the original
materializes `addiu v0,a0,64; sll v0,v0,2`; plus a small number of isolated
register-choice residuals in the tint/draw region.

## Last-resort escalation
`last-resort-decompiler` (GPT-5.6 Sol) invoked with the full dossier
(signature, original word map, 11-variant table, the pin trade-off, and the
fontMeasureString sibling). It recommended the **short-lived unpinned
tied-barrier temporary for the control predicate** (`unsigned int ctrl = *p;
asm volatile("" : "+r"(ctrl)); ctrl -= 8u;`), which is N11/N14 above. Applied
and probed: it fixed the head and the in-place sub-path c-reloads (116→45
diffs) but could not restore the `bnezl`+c-load-delay-slot back-edge. Retain
INCLUDE_ASM; full-ELF parity preserved.

## Re-attempt start point
- experiment20.cpp (clean do-while, correct bnezl, a0 head) — fix the head
  register without a barrier.
- experiment24.cpp (N14, tied barrier, correct head, bnez) — restore the
  bnezl delay-slot c-load without the barrier.
- The distinguishing target is the `bnezl` + next-iter head c-load in the delay
  slot; a candidate must keep that while getting the in-place v0 head.
