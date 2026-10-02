# draw_post_post FontPrintRight (0x1F6940) + dead tail func_001F6928

Matched: `FontPrintRight` (vram 0x1F6940, 35 words) and the 12-byte dead tail
at 0x1F6928 (Splat "function" func_001F6928, 0xA4 bytes total).

## Layout

The old `INCLUDE_ASM func_001F6928` range held two things:

- 0x1F6928..0x1F6940: three `addiu sp,sp,0x60; nop` units — a multi-unit
  dead tail emitted after the parent func_001F6638's (0xD0 frame) RTL. The
  0x60 matches the NEXT function's frame, not the parent's (same shape as the
  989snd dead-tail family). Deadness verified: `tools/deadness_scan.py`
  reports 0 references at 0x1F6928/0x1F6930/0x1F6938; no Ghidra function.
  Preserved as raw asm per the dead-tail policy (glabel func_001F6928).
- 0x1F6940..0x1F69CC: the real function (the Splat `alabel` inside the range
  came from `build/undefined_funcs_auto.txt` detecting the boundary).

## Semantics

Right-aligned font printer, member of the unmangled FontPrint API family:

```c
extern "C" void FontPrintRight(int x, int y, int color, u8* text, int length) {
    FontPrint(x - drawTextSmall((char*)text, length), y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_SMALL), fontSmallGlyphs);
}
```

It measures the string with drawTextSmall (0x1F6250, fontSmallGlyphs widths)
and subtracts the FULL width from x (the text's right edge lands at x). The
Center siblings (FontPrintCenter 0x1F6AF0) do the same with `sra v0,v0,1`
(half width); the plain siblings (FontPrintLarge/Small) do no adjustment.

Naming: the stripped ELF has no symbol at 0x1F6940. Named FontPrintRight
following the project's Center-family convention (plain/Small/Large = effect
slots 1/2/3) and the direct-descendant Deadlocked trio
(FontPrintRight/FontPrintRightSmall/FontPrintRightLarge in
reference/dl/game_dl/fonts.h). The sibling right-aligned wrappers:
0x1F69D0 (fontMediumGlyphs, slot 2 -> FontPrintRightSmall) and
0x1F6A60 (fontLargeGlyphs, slot 3 -> FontPrintRightLarge); both are now
matched (2026-10-02), completing the right-aligned trio. Glyph-width measurement from the original tables confirms the
rendered sizes: fontMediumGlyphs is actually the narrowest font,
fontSmallGlyphs middle, fontLargeGlyphs widest — the wrapper suffixes are
inconsistent across families in the original (see FontPrintSmall's comment).

## Codegen notes

- Register map (from objdump, Splat's `daddu` mnemonics are `move`):
  s0=x, s1=text, s2=length, s3=y, s4=color; frame 0x60.
- `drawTextSmall(text, length)` passes text=length args (a0,a3 / a1,a4);
  after the call a1 is stale and the one-arg `GetEffectTex(1)` reuses it
  without reload — same stale-a1 pattern as FontPrintLarge/Small (the callee
  never reads the second param).
- `x - width` is `subu s0,s0,v0` (x held in s0 across the call, no extra
  local — inline form).
- 7th FontPrint arg (glyphs) in t2 per EGC's 8-arg ABI: `lui t2` hoisted
  right after the GetEffectTex call, `addiu t2` in the FontPrint jal delay
  slot.
- The `(char*)text` cast is codegen-neutral (cfront rejects u8* -> char*
  without it: "changes signedness").

## Related change

Introduced `FONT_EFFECT_TEX_SMALL/MEDIUM/LARGE` (1/2/3, the per-level
effectTexs[] slots) and used them in FontPrintLarge/FontPrintSmall/
FontPrintRight (STYLEGUIDE magic-number rule). Literal-to-define substitution,
verified codegen-neutral by full parity.

## Verification

- Object: FontPrintRight 35/35 words, dead tail 6/6 words (relocations:
  drawTextSmall__FPci, GetEffectTex__Fii, FontPrint, fontSmallGlyphs hi/lo).
- `make split` regenerated the pause_post2 callers from `jal func_001F6940`
  to `jal FontPrintRight` (name from config/symbols.txt).
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf`: byte-identical.
- decomp-verifier subagent: PASS on all mechanical checks (2026-10-02).
- `tools/decomp_status.py --count`: 601 -> 600.
