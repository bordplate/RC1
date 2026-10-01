# FontPrintLarge (0x1F6530, 0x7C) — MATCHED 2026-10-01

`extern "C" void FontPrintLarge(int x, int y, int color, u8* text, int length)`
at `code/game/draw_post_post.cpp:986`. Five-arg wrapper around FontPrint:
prints text with the `fontSmallGlyphs` set and effect texture 1 bound.

## Verified semantics

Forwarding wrapper, natural argument order (no reordering):

```cpp
extern "C" void FontPrintLarge(int x, int y, int color, u8* text, int length) {
    FontPrint(x, y, color, text, length, GetEffectTex(1), fontSmallGlyphs);
}
```

- FontPrint's 7 parameters arrive in a0, a1, a2, a3, t0, t1, t2
  (callee prologue: s3<-a0, s5<-a1, s1<-a2, s0<-a3, s6<-t0, s4<-t1, s2<-t2);
  the wrapper's own 5th parameter (length) arrives in t0 (register 8), so the
  wrapper prologue saves s0<-a0, s1<-a1, s2<-a2, s3<-a3, s4<-t0.
- GetEffectTex(1) result (v0) is forwarded to FontPrint's tex parameter in
  t1; fontSmallGlyphs is materialized as the 7th arg in t2
  (`lui t2,%hi; addiu t2,t2,%lo` in the FontPrint jal delay slot).
- Sibling FontPrintSmall (0x1F65B0) is the same shape with
  `GetEffectTex(2)` + `fontMediumGlyphs` (followed by its 4-byte dead tail
  func_001F6630 `addiu sp,sp,0x60; nop`, a separate EGC artifact).
- Note the glyph-table naming is inverted vs the wrapper name: "Large" uses
  fontSmallGlyphs, "Small" uses fontMediumGlyphs.

## Codegen facts

- The wrapper body is the standard EGC 5-arg-save + two-call shape; no
  pins, barriers, or private flags needed (default -G8 -O2, SN assembler).
- `GetEffectTex` must be declared with ONE parameter plus an `asm` label:
  the real label is the C++-mangled `GetEffectTex__Fii` (2-int function),
  the callee never reads its 2nd param (a1 is used as scratch from
  0x1F4514 on), and the original call materializes only a0 (`li a0,1` in
  the jal delay slot, a1 left stale — verified at all 14 call sites).
  This EGC rejects a one-arg call to a two-parameter prototype
  ("too few arguments to function"), so the 1-param declaration pinned to
  the original label is the only matching form.
- FontPrintLarge and FontPrint are unmangled C-linkage symbols (stripped
  ELF symbol names), matching the rest of the FontPrint family;
  GetEffectTex__Fii is C++-mangled.
- Probe: cand4 (int color) and cand3 (unsigned long color) both 0 word
  diffs; int kept per the project ABI (int and pointers are 32 bits).
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` byte-identical.
