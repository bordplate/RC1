# fontMeasureString (0x001F6200)

Matched 2026-10-01. Size 0x50 (20 words). Leaf, C linkage (unmangled).

## Behavior

Sums the signed advance-width byte of each font-glyph table entry over the
nonzero text bytes, stopping at the first zero byte or when the counter reaches
`length`. Returns the accumulated width.

- Glyph tables (`fontSmallGlyphs`/`fontMediumGlyphs`/`fontLargeGlyphs`): each
  entry is 4 bytes; the signed width is the final byte (offset +3).
- `text` param is `u8*` (the `text[0]` pre-check load is an unsigned `lbu`,
  not `lb`, so the param must be unsigned).
- Callers (`drawTextSmall/Medium/Large`) pass `char*`; the no-op `(u8*)` cast
  keeps them byte-identical.

## Matching form (default flags, SN)

```c
#define FONT_GLYPH_STRIDE 4
#define FONT_GLYPH_WIDTH_OFFSET 3

extern "C" int fontMeasureString(u8* text, int length, char* glyphs) {
    int total = 0;
    int i = 0;

    if (length != 0 && text[0] != 0) {
        u8* p = text;
        u8 c = *p;

        do {
            ++i;
            ++p;
            // Tied barrier: blocks EGC edge-splitting the width index (c*stride)
            // ahead of the pointer advance, preserving the original loop head.
            asm volatile("" : "+r"(c) : "r"(p));

            char w = glyphs[c * FONT_GLYPH_STRIDE + FONT_GLYPH_WIDTH_OFFSET];
            if (w != 0)
                total += w;

            if (i == length)
                break;

            c = *p;
        } while (c != 0);
    }

    return total;
}
```

## Codegen constraints (the hard part)

Two independent EGC 2.95.2 scheduling behaviors had to be defeated:

1. **`sll` edge-splitting before `++p`**: the original loop head is
   `[++i; ++p; sll (width index); ...]`. EGC otherwise edge-splits/hoists the
   `sll` (the `c*4` compute) ahead of the pointer advance. The
   `asm volatile("" : "+r"(c) : "r"(p))` tied barrier between `++p;` and the
   glyph load blocks the `sll` from splitting across the `p` def, forcing the
   original `[p++, sll]` order. (cand19: this alone fixed 2 of 3 residual diffs.)

2. **Constant-fold + edge-split of the counter**: the original has an `i++`
   on BOTH the entry edge (0x1f621c, before the loop) and the latch/backedge
   (0x1f6244). Writing the increment at the prologue (an explicit `++i` before
   the loop, or `int i=1`) lets EGC constant-fold `i=0; i++;` into one
   `li $9,1` and reshuffle the prologue (5-9 diffs; not defeatable by temp or
   pointer indirection — cand13/14/15/20/21). The working form keeps a SINGLE
   `++i` at the TOP of the loop body; with the `sll` now ineligible to split
   (from barrier #1), EGC edge-splits that single `++i` onto the entry edge and
   the latch edge, reproducing both original `i++` copies. The backedge copy
   runs on the `c==0` exit where `i` is dead (harmless); the `i==length` break
   bypasses it. (cand22: 0 diffs.)

The prologue MUST be the if-guarded form `if (length != 0 && text[0] != 0)`
(emits `beq $5,$0` on length + `beq $2,$0` on `text[0]`); a `while (i != length
&& *p != 0)` form emits `beq $9,$5` on `i` and mis-orders the prologue.

## Flags tried

Accepted by EGC 2.95.2 (no effect on the if-guarded form): `-fno-rerun-cse-after-loop`,
`-fno-gcse`, `-fno-peephole`, `-fno-schedule-insns[2]`. `-O1` is far worse
(19 diffs); `-O2` + default scheduler is the correct base. Rejected (probe
fails): `-fno-cse`, `-fno-peephole2`.

## Symbol

`fontMeasureString = 0x001F6200;` added to `config/symbols.txt` (also present in
`config/linker_aliases.ld`). Splat moved the function to
`code/_generated/matchings/game/draw_post_post/fontMeasureString.s` and updated
the referencing generated files (`FontPrintWindow.s`, `pause_post/func_0021B1C8.s`,
and the already-matching `func_001F6250/70/90.s`) from the `func_001F6200`
placeholder to the real name; the orphaned
`code/_generated/nonmatchings/game/draw_post_post/func_001F6200.s` was removed.

## Escalations

`expert` (dossier; suggested tied-operand barriers + the `-fno-*` flags, and
hypothesized loop rotation) and `last-resort-decompiler` (recommended the
single-top-of-body-`++i` + barrier combination = cand22) were both consulted
before the match.

## Verification

Probe: 0 word diffs (cand22). Full `make` + `cmp build/boot_elf.elf
assets/boot_elf.elf` byte-identical. decomp_status count 605 -> 604.
