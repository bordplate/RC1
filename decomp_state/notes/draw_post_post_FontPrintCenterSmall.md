# FontPrintCenterSmall (0x1F6B88, 0x94) — matched 2026-10-02

`code/game/draw_post_post.cpp:1097` (was `INCLUDE_ASM` at line 1097).
Center-aligned member of the unmangled (C linkage) FontPrint API family.

## Semantics

```c
extern "C" int FontPrintCenterSmall(int x, int y, int color, u8* text,
                                    int length) {
    int cx = x - (drawTextMedium((char*)text, length) >> 1);
    FontPrint(cx, y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_MEDIUM), fontMediumGlyphs);
    return cx;
}
```

Byte-identical structural twin of the matched FontPrintCenter (0x1F6AF0):
same 0x60 frame (s0-s4 + ra saved, all five args cached), same
`[a3->a1, a2->a0]` drawText arg swap in the jal delay slot, same `sra`
half-width, same `subu` in the GetEffectTex delay slot, same
[a5=a2-tex, a1=y, a2=color, a3=text, a4=length, a0=cx, t2=glyphs]
FontPrint argument order, same trailing `move v0,s0` return of cx. Only the
font variant differs: drawTextMedium (0x1F6270) instead of drawTextSmall,
effect slot 2 (FONT_EFFECT_TEX_MEDIUM), and the fontMediumGlyphs table
(0x1DF3F0).

Straight-line call-sequence wrapper (no loop) — not subject to the FontPrint
bnezl back-edge wall. First probe matched 148/148 bytes with the Makefile
default flags; full build + `cmp build/boot_elf.elf assets/boot_elf.elf`
byte-identical.
