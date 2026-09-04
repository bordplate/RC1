# draw font-string width wrappers (func_001F6250 / 001F6270 / 001F6290)

Matched: `func_001F6250` (vram 0x1F6250, file offset 0xF71D0, 32 bytes).
Remaining siblings: `func_001F6270` (0x1F6270) and `func_001F6290` (0x1F6290),
both the same shape.

## Semantics

Each wrapper forwards its two arguments to `func_001F6200` (vram 0x1F6200)
and adds a third argument: the address of a glyph-width table.
`func_001F6200(char* idx, int count, char* base)` walks a NUL-terminated
array of u8 glyph indices (bounded by `count`) and returns the sum of the
per-glyph widths stored as signed chars at `base + idx*4 + 3`.
Callers use the result for horizontal centering, e.g. vram 0x1F69D0 does
`x -= width`.

Table mapping (from %hi refs in the generated asm):

| wrapper        | glyph table | also used by                          |
|----------------|-------------|---------------------------------------|
| func_001F6250  | D_001DF050  | FontPrintLarge, func_001F69D0 et al.  |
| func_001F6270  | D_001DF3F0  | FontPrintSmall                        |
| func_001F6290  | D_001DF790  | FontPrintCenterLarge, func_001F6A60   |

## Codegen

Original (0x1F6250):

```
addiu sp,-0x10
lui   a2, %hi(TABLE)
sq    ra, 0(sp)
jal   func_001F6200
addiu a2, a2, %lo(TABLE)   # jal delay slot
lq    ra, 0(sp)
jr    ra
addiu sp, +0x10            # jr delay slot
```

EGC reproduces this exactly from:

```cpp
extern "C" int func_001F6250(char* param_1, int param_2) {
    return func_001F6200(param_1, param_2, D_001DF050);
}
```

The constant third argument is split across the call: `%hi` lands before the
`sq ra`, `%lo` in the `jal` delay slot. Same scheduling family as
snd_CloseMovieSound (constant arg setup interleaved with prologue).

## cfront gotcha

- Passing `&D` where `D` is `extern "C" int D_x;` to an `int` parameter is
  rejected: "passing `int *' to argument 3 ... lacks a cast". The callee
  prototype must take the table as a pointer: `(char*, int, char*)`.
- A scalar `extern "C" char D_x;` does NOT decay in cfront (only arrays do);
  use an array decl `extern "C" char D_001DF050[];` so passing the name
  yields the address.
- The tables are dlabs in `code/_generated/build/data/data.data.s`; their
  link addresses come from `build/undefined_syms_auto.txt`.

## Sibling recipe (for next iterations)

For func_001F6270 / func_001F6290: identical C with D_001DF3F0 / D_001DF790.
Expected words (original): 0x1F6270 = F0FFBD27 1E00063C 0000BF7F 80D8070C
F0F3C624 0000BF7B 0800E003 1000BD27 (lui lo = 0xF3F0), 0x1F6290 same with
hi 0x1E lo 0xF790. Only difference from 0x1F6250 is the %lo addiu immediate;
%hi is 0x1E for all three, and note 0x1DF3F0/0x1DF790 do not need a pseudo
carry (no upper-immediate issue).
