# FontPrintWindowSmall (0x1F6FD0, 0xA0) — matched 2026-10-02

`code/game/draw_post_post.cpp:1149`. Windowed-font member of the FontPrint API
family (C linkage, unmangled; name cross-checked against Deadlocked's
`FontPrintWindowSmall__FP10FontWindowUlPci`).

```cpp
void FontPrintWindowSmall(int x, int y, int width, int height,
                          long color, u8* text, int length) {
    FontPrintWindowGeneric(x, y, width, height, color, text, length,
                           GetEffectTex(FONT_EFFECT_TEX_MEDIUM),
                           (fontLetter*)fontMediumGlyphs);
}
```

## ABI details

- 7 params arrive in a0,a1,a2,a3,t0,t1,t2 (the EGC window a0-a3 + t0-t3);
  the wrapper saves all seven to s0-s6 (arg order) across the GetEffectTex
  call (frame 0x90).
- color is `long` in the wrapper: the caller (message-box renderer at
  0x21F688, call site 0x21F7A8) passes `move t0, s1` from a 32-bit lui/ori
  constant with no extension, and the wrapper forwards `move t0, s4` with no
  extension — an `int` param would force a sign-extend in the call setup.
  FontPrintWindowGeneric then zexts (dsll32/dsra32) in its own prologue.
- The call to FontPrintWindowGeneric (9 args): a0-a3 = x,y,width,height,
  t0-t3 = color,text,length,effect, and the 9th arg (glyphs =
  fontMediumGlyphs, 0x1DF3F0) in the 0(sp) stack slot (`sw v1, 0(sp)`).
  GetEffectTex(2) is scheduled first (jal with li a0,2 in the delay slot);
  its v0 result is forwarded as effect via `move t3, v0` in the final jal's
  delay slot.
- Return: FontPrintWindowGeneric's value (lines used = final line y - y + 0x10)
  passes through in v0 untouched; the caller uses it to decide line advance
  (`slti v0, s0, 0x11` at 0x21F7C0).

## Related changes (same commit)

- `fontLetter` typedef added (glyph-table entry: u8 u, u8 v, s8 drop,
  s8 advance; layout verified against the FontPrint family field accesses,
  see notes/draw_post_func_001F6CB8.md).
- `FontPrintWindowGeneric` prototype with `asm("func_001F6CB8")` override
  (that target is blocked; see decomp_state/blocked.json).
- `FontPrintWindowSmall = 0x1f6fd0;` added to config/symbols.txt (fonts
  section).
- Dead tail func_001F7070 (0x1C) after the wrapper: `addiu sp,sp,0x100` +
  three `addiu sp,sp,0x70; nop` units, 0 references (deadness_scan), no
  Ghidra function — emitted as exact words per the EGC dead-tail rule, with
  the trailing nop padding to FontPrintWindow at 0x1F7090.

## Verification

First probe 160/160 bytes match with default flags
(working/FontPrintWindowSmall/A/candidate.json). Full clean build +
`cmp build/boot_elf.elf assets/boot_elf.elf` byte-identical; status count
595 -> 593.
