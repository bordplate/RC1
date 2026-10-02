# FontPrintCenterLarge (0x1F6C20, 0x94) — matched 2026-10-02

`code/game/draw_post_post.cpp` (was `INCLUDE_ASM` at line 1109).
Center-aligned member of the unmangled (C linkage) FontPrint API family.

## Semantics

```c
extern "C" int FontPrintCenterLarge(int x, int y, int color, u8* text,
                                    int length) {
    int cx = x - (drawTextLarge((char*)text, length) >> 1);
    FontPrint(cx, y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_LARGE), fontLargeGlyphs);
    return cx;
}
```

Byte-identical structural twin of FontPrintCenter (0x1F6AF0) and
FontPrintCenterSmall (0x1F6B88): same 0x60 frame (s0-s4 + ra), same arg
swaps, `sra` half-width, `subu` in the GetEffectTex delay slot,
[a5,a1,a2,a3,a4,a0,t2] FontPrint argument order, trailing `move v0,s0`.
Only the font variant differs: drawTextLarge (0x1F6290), effect slot 3
(FONT_EFFECT_TEX_LARGE), fontLargeGlyphs (0x1DF790).

Straight-line call-sequence wrapper (no loop) — outside the FontPrint bnezl
back-edge wall. First probe matched 148/148 bytes with default flags; full
build + `cmp build/boot_elf.elf assets/boot_elf.elf` byte-identical.
