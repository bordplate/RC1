# draw_post_post FontPrintRightSmall (0x1F69D0)

Matched: `FontPrintRightSmall` (vram 0x1F69D0, 35 words / 0x8C bytes).

## Semantics

Right-aligned font printer, member of the unmangled FontPrint API family —
the medium-glyph variant of FontPrintRight (0x1F6940):

```c
extern "C" void FontPrintRightSmall(int x, int y, int color, u8* text,
                                    int length) {
    FontPrint(x - drawTextMedium((char*)text, length), y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_MEDIUM), fontMediumGlyphs);
}
```

It measures the string with drawTextMedium (0x1F6270, fontMediumGlyphs
widths) and subtracts the FULL width from x (the text's right edge lands at
x). This is the byte-identical sibling of FontPrintRight with the medium
 effect slot (FONT_EFFECT_TEX_MEDIUM = 2) and the fontMediumGlyphs table
 (0x1DF3F0) in place of the small slot/table. The large-glyph right-aligned
 sibling FontPrintRightLarge (0x1F6A60, fontLargeGlyphs, slot 3) is also
 matched (2026-10-02), completing the right-aligned trio.

## Naming

The stripped ELF has no symbol at 0x1F69D0. Named FontPrintRightSmall
following the project's Right/Center-family convention (plain/Small/Large =
effect slots 1/2/3; the "Small" suffix maps to the MEDIUM glyph table, the
same inversion the Center family documents) and the direct-descendant
Deadlocked trio (FontPrintRight / FontPrintRightSmall / FontPrintRightLarge
in reference/dl/game_dl/fonts.h). The FontPrintRight note
(notes/draw_post_post_FontPrintRight.md) already predicted this name for the
0x1F69D0 sibling. Unmangled C-linkage symbol (extern "C").

## Match

Straight-line call-sequence wrapper (no loop), so it is not subject to the
FontPrint bnezl back-edge wall that blocks the loop-based family members
(FontPrint 0x1F62B0, func_001F6638). Written as the direct analog of the
already-matched FontPrintRight (same C form, medium constants). Matched on
the first probe: object disassembly of build/boot_elf.elf at 0x1F69D0 is
word-for-word identical to the generated func_001F69D0.s (prologue save
order, the move s2,t0 / move s1,a3 arg mapping, jal 0x1F6270 / 0x1F44B8 /
0x1F62B0 targets, li a0,2, the lui t2 / addiu t2,-0xC08 fontMediumGlyphs
split, and the lq/jr epilogue). Full make + cmp byte-identical.

## Source-order note

Kept at the same source position as the removed INCLUDE_ASM (immediately
after FontPrintRight, before func_001F6A60) so the intra-TU .text layout is
unchanged and 0x1F69D0 stays pinned by source order (no per-function pin).
