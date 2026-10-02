# FontPrintWindowLarge (0x1F7580, 0x6C) — MATCHED 2026-10-02

`extern "C" void FontPrintWindowLarge(FontWindow* f, long color, u8* text,
int length)` at `code/game/draw_post_post.cpp:1214`. Windowed-font wrapper:
prints text inside a FontWindow with the `fontSmallGlyphs` set and effect
texture 1 (FONT_EFFECT_TEX_SMALL) bound.

## Verified semantics

Forwarding wrapper, natural argument order (no reordering):

```cpp
extern "C" void FontPrintWindowLarge(FontWindow* f, long color, u8* text,
                                     int length) {
    FontPrintWindow(f, color, text, length,
                    GetEffectTex(FONT_EFFECT_TEX_SMALL), (fontLetter*)fontSmallGlyphs);
}
```

- FontPrintWindow's 6 parameters arrive in a0-a3, t0, t1 (see its blocked
  note, notes/draw_post_post_FontPrintWindow.md): f, rgba, text, length in
  a0-a3; tex in t0; glyphs in t1.
- GetEffectTex(1) result (v0) is forwarded to FontPrintWindow's tex
  parameter in t0 with a single 64-bit `move t0, v0`; fontSmallGlyphs is
  materialized as the 6th arg in t1 (`lui t1,%hi` right after the
  GetEffectTex jal, `addiu t1,t1,%lo` in the FontPrintWindow jal delay
  slot) — same split pattern as FontPrintLarge's t2.
- The 4 wrapper params save to s0-s3 (a0-f, a1-color, a2-text, a3-length)
  in the standard EGC save/copy interleave; frame 0x50.
- The glyph-table naming follows the family inversion: "Large" uses
  fontSmallGlyphs (the same set and slot FontPrintLarge binds); the
  "Small" window member (FontPrintWindowSmall, 0x1F6FD0, 7-arg form) uses
  fontMediumGlyphs.
- Name cross-checked against Deadlocked `reference/dl/game_dl/fonts.h`,
  which declares the same 4-arg pair
  `FontPrintWindowSmall(FontWindow*, uint64, char*, int)` /
  `FontPrintWindowLarge(FontWindow*, uint64, char*, int)` (the DL Large
  body is empty, so the glyph set comes from the RC1 bytes).

## Codegen facts

- Exact codegen twin of the matched FontPrintLarge (0x1F6530) with a
  different arg count: 4 params saved (not 5), no length-in-t0 save, tex in
  t0 (not t1), glyphs in t1 (not t2). Default -G8 -O2, SN assembler; no
  pins, barriers, or private flags.
- `long color` / `long rgba` and `long tex` prototypes: EGC 2.95.2 emits
  the plain `move t0, v0` for the int->long promotion of the GetEffectTex
  result (no sll32/dsrl32 sign-extend pair) and 64-bit `move` copies for
  the long color in both prologue and call setup, so the long types are
  codegen-safe and match the FontPrintWindow blocked candidate's
  `(FontWindow* f, long rgba, u8* text, int length, long tex,
  fontLetter* glyphs)` prototype — keep the prototype consistent with that
  candidate for the 0x1F7090 re-attempt.
- The `FontWindow` typedef (12 shorts) moved from below to above the
  function; declarations emit no code, intra-TU layout unchanged (verified
  by full-image parity). A `FontPrintWindow` extern prototype was added at
  the call site (no C definition exists yet; the function is still
  INCLUDE_ASM, blocked on the register-allocation wall).
- `make split` was rerun after adding `FontPrintWindowLarge = 0x1f7580;`
  to config/symbols.txt so the generated nonmatching asm of the 14 callers
  (func_001F4BE0, freeze, pause_post2, ...) references the real name
  instead of func_001F7580.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` byte-identical
  on the first attempt (27/27 words).
