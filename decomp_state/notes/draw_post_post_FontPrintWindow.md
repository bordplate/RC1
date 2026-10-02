# FontPrintWindow (0x1F7090, 0x4F0 = 316 words) — blocked on register allocation

Windowed font printer: wraps `text` into up to 32 width-bounded lines inside a
`FontWindow`, re-splits until the line count / last-line width settle, then
draws each in-window line with `FontPrint` (int) or the float printer
`func_001F6638` (flags bit 3). C linkage, unmangled symbol `FontPrintWindow`.

## Status: BLOCKED (register-allocation wall)

Logic is fully reverse-engineered and reproduced in C (candidate in the
Appendix below).
With the candidate, the object is 315 words (1 short of 316) at the correct
frame (0x190). The residual is a pervasive, coupled register-allocation
difference, not a logic or size difference. Same wall class as the already
blocked sibling `FontPrintWindowGeneric` (func_001F6CB8).

## Verified logic (reproduces the original semantics)

- Params (EGC EE ABI arg window a0,a1,a2,a3,t0,t1,...):
  a0=f, a1=rgba(long), a2=text, a3=length, t0=tex(long), t1=glyphs.
- Prologue: frame 0x190; save s0-s8, ra, f20; spill rgba to 192(sp), tex to
  200(sp); the original keeps f=s0, text=s4, length=s3, glyphs=s6 (4 params
  callee-saved) and reuses the rest for locals.
- `VU1_setScissor(f->w, f->h-1, f->x, f->y-1)` (FontWindow offsets
  x=0,y=2,w=4,h=6,textX=8,textY=10,maxTextH=12,totalH=14,lineH=16,flags=18,
  offX=20,offY=22).
- `fontCtrlColorsInstalledGp = 1;` is a GPREL store (`sw v, -0x7760(gp)`) in
  the `beqz` delay slot (always). Uses the `.extern`-seeded alias
  `fontCtrlColorsInstalledGp` (linker_aliases.ld).
- maxwidth: `if ((flags^1)&1) maxwidth = h - textX; else maxwidth = min(h-textX,
  textX-w) << 1;` then `maxwidth_orig = maxwidth` (the original stores
  maxwidth_orig to 208(sp) and restores it on the `ideal < line_count` retry:
  `li s8,1; lw a1,208(sp); b retry`).
- Split do-while: per line record begs/rgbas; char loop breaks on c<2, records
  end-candidate on space/ctrl, sets `rgba_idx = c-8` when occl && (c-8u)<8, and
  accumulates width via `idx = *line_ends[line_count]` (a rolling byte, NOT the
  char — verified against the disasm; on pass 1 it reads uninitialized stack).
- Re-split decision: `accept` break / `line_count<2` break / `ideal==0 -> ideal
  = line_count` / `ideal < line_count -> {accept=1; maxwidth=maxwidth_orig;
  resplit}` / `last_width < maxwidth/3 -> {maxwidth -= 0x10; resplit}`.
- Post: `h = line_count * lineH; maxTextH = 0; y = textY; if (flags&2) {
  totalH = h; y -= h>>1; }` (totalH store is in the beqz delay slot, always).
- Draw loop: skip off-window lines; `width = fontMeasureString(text+begin,
  span, glyphs)`; update maxTextH; if !(flags&4): `fontCtrlColors[0] = rgba`
  (absolute array store); color = `fontCtrlColors[line_rgbas[line]]`;
  flags&8 -> float printer (`q = 0.0625f`, fx centered if flags&1), else
  FontPrint (centered if flags&1).
- Epilogue: `fontCtrlColorsInstalled = 0;` (ABSOLUTE store, plain symbol) then
  `VU1_setScissor(0, occlViewParams.paramX-1, 0, occlViewParams.paramY-1)`.

## Register-allocation wall (the blocker)

Original param/local allocation vs EGC 2.95.2 (default flags):
- original: f=s0, text=s4, length=s3, glyphs=s6, line_count=s5, accept=s8,
  maxwidth=a1, ideal=s1, y=s1; maxwidth_orig=208(sp).
- EGC:     f=s0, text=s6, length=s2, glyphs=SPILL, line_count=s8, accept=s7,
  maxwidth=t2, y=s2.
~298/316 words differ; the difference is the register map, not the RTL.

Attempted and measured (object = `make build/code/game/draw_post_post.o`,
diff = `tools/tu_assembler_diff.py`):
- default flags: 315 words (1 under), 297 word diffs.
- `-fno-schedule-insns`: 316 words (size matches), 298 word diffs.
- `-fno-schedule-insns2`: 316 words (size matches), 298 word diffs.
- pin glyphs->s6 (`register fontLetter* pg asm("$22")`): 321 words (worse; the
  pin frees an s-reg that EGC then spills into elsewhere).
- pin f->s0 + text->s4 + length->s3 + glyphs->s6: 323 words (worse).
Pinning makes it worse (soft pins cascade spills); scheduling flags fix the
size but not the map. No source form / pin / barrier / TU flag found that
reproduces the original's 4-callee-saved-param allocation without regressing
the local allocation.

## Last-resort

last-resort-decompiler (GPT-5.6 Sol) was invoked for this exact target in the
prior session (task ses_f03830e4dfferWg3HUKYJGhjdH) while the candidate was
still at frame 0x180 / 1128 B. Its recommendation #1 — restore the preserved
original width (`maxwidth_orig`) on the `ideal_lines < line_count` retry
instead of just `accept=1; continue;` — was the key logic correction: it fixed
the frame (0x180 -> 0x190) and brought the object to 315 words. Its
recommendation #2 (glyph index / persistent text pointer) was a misread of the
branch-likely delay slot (the original DOES use `line_ends[lc]` as the rolling
glyph index) and was discarded. The residual register-allocation wall above
remains after rec #1.

## Recovery note

The C body + shared context were uncommitted at the time of this session and
were lost to a `git checkout`; they were reconstructed from the session history
and the generated original `matchings/game/draw_post_post/FontPrintWindow.s`
(ground truth: 0x4F0, prologue `move s6,t1` / `move s4,a2` / `move s3,a3` /
`move s0,a0`). The reconstructed candidate (FontWindow struct + fontCtrl*
externs + func_001F6638 proto + VU1_setScissor proto + body) compiles clean and
is the best candidate to date. Shared context the candidate needs in config:
`fontCtrlColors = 0x0018CAF8`, `fontCtrlColorsInstalled = 0x0015F4A0`
(symbols.txt), `fontCtrlColorsInstalledGp = 0x0015F4A0` (linker_aliases.ld,
seeded GPREL alias for the prologue store). `FontPrint`'s tex param must be
`long` (the original passes it via a plain `ld`, no truncation); verified the
matched FontPrint* wrappers still pass with the long-tex prototype.

Retain INCLUDE_ASM (parity preserved). Re-attempt start point: the candidate
in the Appendix below; the only gap is the register map, and the sibling
FontPrintWindowGeneric note (blocked.json) documents the same coupled pre-RA
scheduling/allocation wall in this font family.

## Appendix: best candidate C body (was working/fontprintwindow/candidate_C_body.txt)

The candidate is the best-verified form to date (315 words, correct 0x190 frame;
only the register map differs). Config it needs if revisited: symbols.txt
`fontCtrlColors = 0x0018CAF8`, `fontCtrlColorsInstalled = 0x0015F4A0`; linker_aliases.ld
`fontCtrlColorsInstalledGp = 0x0015F4A0` (seeded GPREL alias for the prologue
store); FontPrint tex param = long.

```cpp
typedef struct FontWindow {
    short x;
    short y;
    short w;
    short h;
    short textX;
    short textY;
    short maxTextH;
    short totalH;
    short lineH;
    short flags;
    short offX;
    short offY;
} FontWindow;

extern int fontCtrlColors[16];
extern int fontCtrlColorsInstalled;
extern int fontCtrlColorsInstalledGp;

void VU1_setScissor(int, int, int, int) asm("VU1_setScissor__Fiiii");

// Float variant of the windowed font printer (blocked target func_001F6638):
// three floats (f12-f14) plus five ints (a0-a3, t0).
extern "C" void func_001F6638(float x, float y, float scale, int color,
                              u8* text, int length, long tex,
                              fontLetter* glyphs);

// C linkage: this font API entry point retains the original unmangled name.
// Wraps `text` into up to 32 width-bounded lines within the FontWindow,
// re-splitting until the line count / last-line width settle, then draws each
// in-window line with FontPrint (integer) or the float printer (flags bit 3).
extern "C" void FontPrintWindow(FontWindow* f, long rgba, u8* text, int length,
                                long tex, fontLetter* glyphs) {
    short line_begs[32];
    short line_ends[32];
    short line_rgbas[32];

    VU1_setScissor(f->w, f->h - 1, f->x, f->y - 1);

    int maxwidth;
    asm(".extern fontCtrlColorsInstalledGp, 4");
    fontCtrlColorsInstalledGp = 1;
    if ((f->flags ^ 1) & 1) {
        maxwidth = f->h - f->textX;
    } else {
        int a = f->h - f->textX;
        int b = f->textX - f->w;
        maxwidth = (a < b ? a : b) << 1;
    }

    int maxwidth_orig = maxwidth;

    int rgba_idx = 0;
    int accept = 0;
    int ideal_lines = 0;
    int last_width = 0;

    int line_count;
    do {
        line_count = 0;
        int i = 0;
        if (length != 0 && text[0] != 0) {
            for (;;) {
                line_begs[line_count] = i;
                line_rgbas[line_count] = rgba_idx;
                int end_candidate = i;
                int width = 0;
                if (maxwidth > 0) {
                    int occl = drawOcclusionEnabled;
                    for (;;) {
                        unsigned char c = text[i];
                        if (c == 0x20 || c < 0x10)
                            end_candidate = i;
                        if (occl && (c - 8u) < 8)
                            rgba_idx = c - 8;
                        if (c < 2)
                            break;
                        i++;
                        u8* q = (u8*)&line_ends[line_count];
                        unsigned char idx = *q;
                        int adv = glyphs[idx].advance;
                        if (adv != 0)
                            width += adv;
                        if (width >= maxwidth)
                            break;
                        q++;
                    }
                }
                line_ends[line_count] = end_candidate;
                if (end_candidate != line_begs[line_count])
                    line_ends[line_count] = i;
                int end = line_ends[line_count];
                if (text[end] == 0x20 || (unsigned char)text[end] < 0x10)
                    line_ends[line_count] = end - 1;
                line_count++;
                if (text[end] == 0) {
                    last_width = width;
                    break;
                }
                i = end + 1;
                if (i == length || text[i] == 0)
                    break;
            }
        }
        if (accept)
            break;
        if (line_count < 2)
            break;
        if (ideal_lines == 0)
            ideal_lines = line_count;
        if (ideal_lines < line_count) {
            accept = 1;
            maxwidth = maxwidth_orig;
            continue;
        }
        if (last_width < maxwidth / 3) {
            maxwidth -= 0x10;
            continue;
        }
        break;
    } while (1);

    int h = line_count * f->lineH;
    f->maxTextH = 0;
    int y = f->textY;
    if (f->flags & 2) {
        f->totalH = h;
        y -= h >> 1;
    }
    if (line_count > 0) {
        int line = 0;
        float q = 0.0625f;
        while (line < line_count) {
            if ((y + f->lineH) < f->x || f->y < y) {
                y += f->lineH;
                line++;
                continue;
            }
            int begin = line_begs[line];
            int span = line_ends[line] - begin + 1;
            int width = fontMeasureString(text + begin, span,
                                          (char*)glyphs);
            if (width > f->maxTextH)
                f->maxTextH = width;
            if (!(f->flags & 4)) {
                fontCtrlColors[0] = (int)rgba;
                if (f->flags & 8) {
                    float fx;
                    if (f->flags & 1)
                        fx = (float)(f->textX - (width >> 1)) +
                             (float)f->offX * q;
                    else
                        fx = (float)f->textX + (float)f->offX * q;
                    float fy = (float)y + (float)f->offY * q;
                    func_001F6638(fx, fy, 1.0f,
                                  fontCtrlColors[line_rgbas[line]],
                                  text + begin, span, tex, glyphs);
                } else if (f->flags & 1) {
                    FontPrint(f->textX - (width >> 1), y,
                              fontCtrlColors[line_rgbas[line]],
                              text + begin, span, tex, (char*)glyphs);
                } else {
                    FontPrint(f->textX, y, fontCtrlColors[line_rgbas[line]],
                              text + begin, span, tex, (char*)glyphs);
                }
            }
            y += f->lineH;
            line++;
        }
    }

    fontCtrlColorsInstalled = 0;
    VU1_setScissor(0, occlViewParams.paramX - 1, 0, occlViewParams.paramY - 1);
}
```
