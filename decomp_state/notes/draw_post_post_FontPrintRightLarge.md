# draw_post_post FontPrintRightLarge (0x1F6A60)

Matched: `FontPrintRightLarge` (vram 0x1F6A60, 35 words / 0x8C bytes).

## Semantics

Right-aligned font printer, member of the unmangled FontPrint API family —
the large-glyph variant of FontPrintRight (0x1F6940):

```c
extern "C" void FontPrintRightLarge(int x, int y, int color, u8* text,
                                    int length) {
    FontPrint(x - drawTextLarge((char*)text, length), y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_LARGE), fontLargeGlyphs);
}
```

It measures the string with drawTextLarge (0x1F6290, fontLargeGlyphs widths)
and subtracts the FULL width from x (the text's right edge lands at x). This
is the byte-identical sibling of FontPrintRight / FontPrintRightSmall with
the large effect slot (FONT_EFFECT_TEX_LARGE = 3) and the fontLargeGlyphs
table (0x1DF790). With this the three right-aligned siblings
(FontPrintRight 0x1F6940, FontPrintRightSmall 0x1F69D0, FontPrintRightLarge
0x1F6A60) are all matched, mirroring the matched Center trio layout
(FontPrintCenter 0x1F6AF0, FontPrintCenterSmall 0x1F6B88,
FontPrintCenterLarge 0x1F6C20 — still INCLUDE_ASM).

The six call sites are all in DrawItemsMenu (0x21EB20, the pause/items
screen drawer): four status-line pairs at x = g_itemsMenuPos+0xF0 / 0xF0 with
y offsets 0x1D/0x36/0x54, alternating the 0x80000000 and
0x80FF0000|0xA888 colors, each with length -1.

## Naming

The stripped ELF has no symbol at 0x1F6A60. Named FontPrintRightLarge
following the project's Right/Center-family convention (plain/Small/Large =
effect slots 1/2/3; the "Large" suffix maps to the LARGE glyph table, the
same inversion the other families document) and the direct-descendant
Deadlocked trio (FontPrintRight / FontPrintRightSmall / FontPrintRightLarge
in reference/dl/game_dl/fonts.h). The FontPrintRight note
(notes/draw_post_post_FontPrintRight.md) predicted this exact name for the
0x1F6A60 sibling. The Lombyte reference
(reference/Lombyte/src/textbin/fun_001f6a60.c) confirms the body:
`arg0 - FUN_001f6290(arg3, arg4)` then FontPrint with FUN_001f44b8(3) and
D_001DF790. Unmangled C-linkage symbol (extern "C").

## Match

Straight-line call-sequence wrapper (no loop), so it is not subject to the
FontPrint bnezl back-edge wall that blocks the loop-based family members
(FontPrint 0x1F62B0, func_001F6638). Written as the direct analog of the
already-matched FontPrintRight (same C form, large constants). Matched on
the first probe: object words differ from the original only at the five
relocation sites (jal drawTextLarge 0x1F6290, jal GetEffectTex__Fii
0x1F44B8, lui t2 / addiu t2 fontLargeGlyphs 0x1DF790 split, jal FontPrint
0x1F62B0); zero other diffs. decomp-verifier independently confirmed
0/35 non-relocation word diffs, the relocation resolutions, and byte-for-byte
full boot ELF parity.

## Source-order note

Kept at the same source position as the removed INCLUDE_ASM (immediately
after FontPrintRightSmall, before FontPrintCenter) so the intra-TU .text
layout is unchanged and 0x1F6A60 stays pinned by source order (no
per-function pin). The new symbol name was added to config/symbols.txt so
the generated linker script and the DrawItemsMenu call sites reference
FontPrintRightLarge instead of func_001F6A60.
