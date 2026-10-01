# FontPrintSmall (0x1F65B0, 0x7C) — MATCHED 2026-10-01

`extern "C" void FontPrintSmall(int x, int y, int color, u8* text, int length)`
at `code/game/draw_post_post.cpp:994`. Five-arg wrapper around FontPrint:
prints text with the `fontMediumGlyphs` set and effect texture 2 bound.

## Verified semantics

Byte-identical sibling of the matched FontPrintLarge (0x1F6530): the only
differences are the `GetEffectTex` argument (`li a0,2` vs `li a0,1`) and the
glyph-table pointer (`fontMediumGlyphs` 0x1DF3F0 vs `fontSmallGlyphs`
0x1DF050). Note the glyph-table naming is inverted vs the wrapper name:
"Large" uses fontSmallGlyphs, "Small" uses fontMediumGlyphs.

```cpp
extern "C" void FontPrintSmall(int x, int y, int color, u8* text, int length) {
    FontPrint(x, y, color, text, length, GetEffectTex(2), fontMediumGlyphs);
}
```

- FontPrint's 7 parameters arrive in a0, a1, a2, a3, t0, t1, t2 (see
  notes/draw_post_post_FontPrintLarge.md for the register mapping); the
  wrapper's 5th parameter (length) arrives in t0 and is saved to s4.
- GetEffectTex(2) result (v0) is forwarded to FontPrint's tex parameter in
  t1; fontMediumGlyphs is materialized as the 7th arg in t2
  (`lui t2,%hi; addiu t2,t2,%lo` in the FontPrint jal delay slot).
- Unmangled C-linkage symbol (stripped-ELF name), like the rest of the
  family; GetEffectTex is declared 1-param pinned to `GetEffectTex__Fii`
  (shared declaration with FontPrintLarge, already in the file above).

## Dead tail func_001F6630 (0x1F6630, 0x4)

`addiu sp,sp,0x60; nop` immediately after the wrapper's epilogue (plus the
alignment nop at 0x1F662C). Dead: tools/deadness_scan.py reports 0
references; no Ghidra function. Matches the parent's 0x60 frame — an EGC
artifact emitted after the parent's RTL. Replaced the INCLUDE_ASM with raw
inline asm (`.align 3` + `.word 0x27BD0060` + `.word 0x00000000`) per the
dead-tail policy; the trailing nop pads to the 8-aligned func_001F6638
entry. Not a blocker, not recorded in blocked.json.

## Verification

- `make` + objdump of 0x1F65B0-0x1F6638 in build vs original: identical.
- Full `cmp build/boot_elf.elf assets/boot_elf.elf` byte-identical.
- No pins, barriers, or private flags (default -G8 -O2, SN assembler).
