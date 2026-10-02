# FontPrintCenter (0x1F6AF0, 0x94) — matched 2026-10-02

`code/game/draw_post_post.cpp:1090` (was `INCLUDE_ASM` at line 1086).
Center-aligned member of the unmangled (C linkage) FontPrint API family.

## Semantics

```c
extern "C" int FontPrintCenter(int x, int y, int color, u8* text, int length) {
    int cx = x - (drawTextSmall((char*)text, length) >> 1);
    FontPrint(cx, y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_SMALL), fontSmallGlyphs);
    return cx;
}
```

Prints the string centered on `x` (x minus half the measured glyph width)
with the fontSmallGlyphs set and effect-tex slot 1 (FONT_EFFECT_TEX_SMALL),
and RETURNS the centered x.

## Key discoveries

### 1. The function returns int (the centered x), not void

The epilogue's trailing `move v0,s0` (at 0x1F6B60, after the FontPrint
jal's delay slot) is the return of `cx`. A `void` candidate compiles 4 bytes
short and is missing it. This matches the direct-descendant Deadlocked
`fonts.cpp:2043`: `int FontPrintCenter(int xpos, int ypos, uint64 rgba,
char *s, int len) { ...; return xpos - (iVar1 >> 1); }`.

### 2. `>> 1`, not `/ 2`

The original shifts the measured width with a single in-place
`sra v0,v0,1` (0x1F6B2C). EEGCC compiles a signed `/ 2` as the 3-instruction
floor-division idiom `srl $at,$v0,31; addu $at,$at,$v0; sra $at,$at,1`
(4 bytes too big). A signed `>> 1` emits the single `sra`.

### 3. Argument order follows the int return

With `int cx = ...; FontPrint(cx, ...); return cx;` EGC schedules the
FontPrint argument copies as `[a5=tex(v0), a1=y, a2=color, a3=text,
a4=length, a0=cx(s0)]` — the return value (s0) copied last, right before
the jal. The void/inline form copies `[a0..a5]` in index order and drops
the trailing move.

### 4. Delay-slot subu

`cx = x - (width >> 1)` depends only on the drawTextSmall result, but EGC
schedules the `subu s0,s0,v0` into the GetEffectTex jal's DELAY SLOT (runs
before the callee clobbers v0), reusing v0 = width/2 from the preceding
`sra` and avoiding any stack save of the shifted width.

## Verification

- `tools/decomp_probe.py` standalone: 148/148 byte match
  (variants: `/2` 3-instr idiom fail; `>>1` void form 144B a0/a5-swap fail;
  `>>1` int-return form exact match).
- `tools/fdiff.py 0x1F6AF0 0x94`: 0 word diffs of 37.
- `make split && make -j2 && cmp build/boot_elf.elf assets/boot_elf.elf`:
  byte-identical.

## Sibling status

- FontPrintCenterSmall (0x1F6B88): same shape, drawTextMedium (0x1F6270) +
  GetEffectTex(2) + fontMediumGlyphs.
- FontPrintCenterLarge (0x1F6C20): same shape, drawTextLarge + GetEffectTex(3)
  + fontLargeGlyphs.
- The Right-aligned trio (FontPrintRight/RightSmall/RightLarge,
  0x1F6940/0x1F69D0/0x1F6A60) is matched (void, full-width subtract, no
  sra, no trailing move).
- The 7-arg FontPrint itself (0x1F62B0) remains INCLUDE_ASM/blocked
  (see notes/draw_post_post_FontPrint.md).
