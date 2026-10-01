# func_001F6638 (0x1F6638, 0x2EC) — blocked

`code/game/draw_post_post.cpp:1018` (INCLUDE_ASM). Font glyph-string printer in the
FontPrint family; the LARGER 3-way-branch member (control / tint / normal, plus an
optional extended-glyph sprite). 187 words, frame 0xD0.

## Verified ABI (last-resort corrected)
```
extern "C" void func_001F6638(int color, u8* text, int length,
                              unsigned long long packed,
                              float x, float y, float scale);
```
- a0=colorLow, a1=text, a2=length, **a3:a4=packed (u64)** with
  `colorHigh=(int)packed` (low32) and `glyphs=(FontGlyph*)(packed>>32)` (high32);
  f12=x, f13=y, f14=scale.
- **Why u64:** the caller (FUN_001f7090 @0x1F7470) does `ld a3,200(sp)` (64-bit load
  into a3:a4) + `move t0,s6`. My EGC arg window is **a0-a3 + t0-t3** (a plain 5th int
  arg lands in **t0**, verified with 5/6-int probes), but the original keeps glyphs in
  a4 — so the source must have had a u64 4th arg. My EGC passes a u64 in a3:a4 only
  when BOTH halves are used (a u64 whose high half is never read is zero-extended into
  a3 alone).
- `FontGlyph = struct { u8 u; u8 v; s8 dropV; s8 advance; }` (0/1 `lbu`, 2/3 `lb`).
- Callees: `func_001FA6C0` (int->float, 5 calls) and `func_001F5808` = DrawOcclSprite2
  (3 calls). DrawOcclSprite2 verified 10-arg sig `(float x,y,w,h, s32 u,v,tw,th, u64
  color, u64 tex)`; func_001F6638 passes `color` as the u64 in **a4:a5** =
  `(colorHigh<<32)|colorLow` (or `...|tint`), and does NOT set tex (t0/t1).
- Globals: `fontCtrlColorsInstalled` 0x15F4A0, `fontCtrlColors[8]` 0x18CAF8,
  `drawOcclusionEnabled` 0x15F49C (GPREL).

## Verified semantics
- `if (!fontCtrlColorsInstalled) fontCtrlColors[0]=colorLow;`
- `count=0; scaled16=scale*16.0f;`
- `if (length && *text) { p=text; do { u32 ctrl=*p; ctrl-=8u;`
  - `if (ctrl<8u)` (control): `if (drawOcclusionEnabled) colorLow=(colorLow&0xFF000000)|
    ((int)fontCtrlColors[*p-8]&0xFFFFFF);` (in-place `and`/`or`)
  - `else` (glyph): `if (glyphs[*p].advance) { drop=(float)func_001FA6C0(dropV)*scale;`
    - `if ((u8)(*p+0x80)<0x28)` (extended): ext=glyphs[*p+0x40];
      DrawOcclSprite2(x+adv*scale, y+dropV*scale, scaled16, scaled16, ext.u, ext.v,
      0x10,0x10, (colorHigh<<32)|colorLow)
    - `if (*p<0x20)` (tint): sum=(colorLow&0xFF)+((colorLow>>8)&0xFF)+((colorLow>>16)&0xFF);
      avg=sum/3 (div + `beql s3,0`/`break 0,7` guard, s3=3);
      tint=avg+(colorLow&0xFF000000)+(avg<<16)+(avg<<8);
      DrawOcclSprite2(x, y+drop, scale*24, scale*16, u, v, 0x18,0x10, (colorHigh<<32)|tint)
    - `else if (*p>0x20)` (normal): DrawOcclSprite2(x, y+drop, scaled16, scaled16, u, v,
      0x10,0x10, (colorHigh<<32)|colorLow)
    - `x += (float)func_001FA6C0(glyphs[*p].advance)*scale;`
  - `count++; if (count==length) break; p++;` `} while (*p != 0); }`
- Back-edge (word 167, 0x5440FF84) is **bnezl** (0x15) with the next-iter head c-load in
  the delay slot. (An earlier "bnel" reading was a decoding error — candidate also emits
  bnezl, so there is no opcode wall.)

## Last-resort corrections (applied, each verified to matter)
1. `((int)*p+0x80)&0xFF<0x28` precedence bug (`x & (0xFF<0x28)` = always false) compiled
   the ENTIRE extended path out → size deficit. Fixed to `(u8)(*p+0x80)<0x28`.
2. color/tex for DrawOcclSprite2 are 64-bit (u64 in a4:a5).
3. Glyph fields mixed signedness; control predicate unsigned (`u32 ctrl; ctrl-=8u` →
   `sltiu`); control-color update = two statements (`colorLow&=; colorLow|=`).
4. **`volatile u8* p`** (my addition beyond the model): defeats CSE of the head c-load so
   the sub-paths reload `*p` like the original (plain p CSEs c into a copy `move v1,v0`).

## Candidates (decomp_probe.py; --define fontCtrlColorsInstalled=0x15F4A0
   fontCtrlColors=0x18CAF8 func_001F5808=0x1F5808)
| form | size | diffs |
|---|---|---|
| cand4 u64 ABI, plain p | 716 B | 176 (head CSE `move v1,v0`) |
| cand5 cand4 + tied barrier on ctrl | 716 B | 180 |
| **cand6 cand4 + `volatile u8* p`** | **756 B** | **145 (best)** |
| cand7 cand6 + 64-bit tint sum | 768 B | 145 |
| cand6 + `-fno-schedule-insns` | 756 B | 140 |

Structural flow is IDENTICAL (same 7 jals, same control flow, same frame/ABI). Residual is
pervasive EGC 2.95.2 prologue save-order + body RA/scheduler tie-breaks: prologue `sq`
order differs, head lacks the original's trailing `nop`, extended base is `sll *4; addiu
+256` vs the original's `addiu c+0x40; sll *4`, tint sum is 32-bit `sra/addu` vs the
original's 64-bit `dsrl/daddu` (forcing u64 in C only ADDS instructions), and which GPR/FPR
holds colorLow/mask/float-temps/call-args differs throughout. No pin/barrier/flag (tried
`+r` barrier, volatile p, -fno-schedule-insns[2]) closes it. Same residual class as blocked
FontPrint (0x1F62B0) and DrawOcclSprite2 (0x1F5808).

## Last-resort escalation
`last-resort-decompiler` (GPT-5.6 Sol) invoked with the full dossier (ABI, original word
map, two early candidates + diffs, W1-W4). It diagnosed the `& 0xFF<0x28` precedence bug,
the bnel/bnezl decoding error, the 64-bit color/tex args, the mixed-signedness glyph
fields, the unsigned control predicate, and the two-statement control-color update — all
applied. Recommended testing the corrected unbarriered form first, then one tied barrier
on `ctrl` if the head still came out fresh-register. Both probed (cand4, cand5); neither
matched. Retain INCLUDE_ASM; full-ELF parity preserved.

## Re-attempt start point (best form = cand6)
The best candidate is cand6 = the source below (volatile `u8* p`). A re-attempt should
start here and attack the prologue save-order + body RA (the head/back-edge are already
correct under volatile p).
```cpp
typedef struct FontGlyph { u8 u; u8 v; s8 dropV; s8 advance; } FontGlyph;
extern int fontCtrlColorsInstalled;
extern int fontCtrlColors[8];
extern int drawOcclusionEnabled;
extern "C" float func_001FA6C0(int value);
extern "C" void func_001F5808(float x, float y, float w, float h,
                              int u, int v, int tw, int th, unsigned long long color);
extern "C" void func_001F6638(int color, u8* text, int length,
                              unsigned long long packed,
                              float x, float y, float scale) {
    int colorLow = color;
    int colorHigh = (int)packed;
    FontGlyph* glyphs = (FontGlyph*)(packed >> 32);
    if (fontCtrlColorsInstalled == 0)
        fontCtrlColors[0] = colorLow;
    int count = 0;
    float scaled16 = scale * 16.0f;
    if (length != 0 && *text != 0) {
        volatile u8* p = text;
        do {
            u32 ctrl = *p;
            ctrl -= 8u;
            if (ctrl < 8u) {
                if (drawOcclusionEnabled != 0) {
                    colorLow &= 0xFF000000;
                    colorLow |= fontCtrlColors[*p - 8] & 0xFFFFFF;
                }
            } else {
                FontGlyph* glyph = &glyphs[*p];
                if (glyph->advance != 0) {
                    float drop = func_001FA6C0((int)glyph->dropV) * scale;
                    if ((u8)(*p + 0x80) < 0x28) {
                        FontGlyph* ext = &glyphs[*p + 0x40];
                        float extAdvance = func_001FA6C0((int)ext->advance) * scale;
                        float extDrop = func_001FA6C0((int)ext->dropV) * scale;
                        func_001F5808(x + extAdvance, y + extDrop,
                                      scaled16, scaled16,
                                      ext->u, ext->v, 0x10, 0x10,
                                      ((unsigned long long)colorHigh << 32) | (unsigned long long)colorLow);
                    }
                    if (*p < 0x20) {
                        int sum = (colorLow & 0xFF)
                                + ((colorLow >> 8) & 0xFF)
                                + ((colorLow >> 16) & 0xFF);
                        int avg = sum / 3;
                        int tint = avg + (colorLow & 0xFF000000)
                                 + (avg << 16) + (avg << 8);
                        func_001F5808(x, y + drop,
                                      scale * 24.0f, scale * 16.0f,
                                      glyphs[*p].u, glyphs[*p].v,
                                      0x18, 0x10,
                                      ((unsigned long long)colorHigh << 32) | (unsigned long long)tint);
                    } else if (*p > 0x20) {
                        func_001F5808(x, y + drop, scaled16, scaled16,
                                      glyphs[*p].u, glyphs[*p].v,
                                      0x10, 0x10,
                                      ((unsigned long long)colorHigh << 32) | (unsigned long long)colorLow);
                    }
                    x += func_001FA6C0((int)glyphs[*p].advance) * scale;
                }
            }
            ++count;
            ++p;
            if (count == length)
                break;
        } while (*p != 0);
    }
}
```
