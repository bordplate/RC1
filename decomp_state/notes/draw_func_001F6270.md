# func_001F6270 (vram 0x1F6270, file 0xF71F0, 32 bytes) — MATCHED 2026-09-04

Font-string width calculator for the D_001DF3F0 glyph table: pass-through
wrapper `func_001F6270(a0, a1) -> return func_001F6200(a0, a1, D_001DF3F0);`.

Exact sibling of matched func_001F6250 (D_001DF050 table); the func_001F6250
matched.json entry already flagged both siblings ("byte-identical shape with
only the global differing - decompile with the same pattern").

## Identity / context

func_001F6200 (vram 0x1F6200, still INCLUDE_ASM) walks a NUL-terminated u8
index array bounded by count and sums per-glyph widths read from the table
(char at idx*4+base+3); callers use the result to center text. Ghidra
decompiles func_001F6270 as `FUN_001f6200(param_1, param_2, 0x1df3f0)`; the
assembly sets only $a3 because $a0/$a1 are forwarded in-register (no moves
emitted). All 8 call sites load both args before the `jal`
(e.g. 0x21F3AC: `li a1,-1; jal func_001F6270` with a0 = string from the
previous delay slot; 0x1F6A04: `move a1,s2; jal func_001F6270`), and callers
consume $v0 (`move a0,v0`), so the wrapper returns the callee's int.

D_001DF3F0 is the small-font glyph-width table (dlabel in
build/data/data.data.s, %hi 0x1E000 / %lo -0xC10). Declared in draw.cpp as
`extern "C" char D_001DF3F0[];` — same cfront gotcha as the func_001F6250
note: the callee's third param must be a pointer type, so the prototype is
`(char*, int, char*)` and the data is a char array, not `int` + `&D`.

## Source

Replaced the INCLUDE_ASM in code/game/draw.cpp (was line 126) with:

```cpp
extern "C" char D_001DF3F0[];

extern "C" int func_001F6270(char* param_1, int param_2) {
    return func_001F6200(param_1, param_2, D_001DF3F0);
}
```

Kept the original `func_001F6270` symbol name (project convention for C
symbols; final ELF is stripped so the name does not affect the binary).

## Verification

- EGC emits the documented wrapper shape: `addiu sp,-0x10 / lui a3,%hi /
  sq ra,0(sp) / jal func_001F6200 / <ds> addiu a3,a3,%lo / lq ra / jr ra /
  <ds> addiu sp,+0x10` — the constant-arg materialization split (hi before
  sq, lo in the jal delay slot) is reproduced exactly.
- Built image slice file 0xF71F0..0xF7210:
  `f0ffbd27 1e00063c 0000bf7f 80d8070c f0f3c624 0000bf7b 0800e003 1000bd27`
  byte-identical to the original.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- decomp_status --count: 830 -> 829.

Sibling func_001F6290 (D_001DF790 table) remains INCLUDE_ASM; same pattern
applies.
