# fontTextSubmitCentered (code/game/draw.cpp) — MATCHED 2026-09-25

`int fontTextSubmitCentered(int x, int y, int field_0x08, unsigned char* text)`
at vram `0x001F0C50`, 140 bytes (0x8C), symbol `func_001F0C50`. The Splat
placeholder `func_001F0C48` actually spans 0x1F0C48–0x1F0CDC: the first 8 bytes
(0x1F0C48–0x1F0C4C) are a dead tail that belongs to the preceding
`fontTextSubmit`, and the real function starts at 0x1F0C50.

## Identity / context

- Measures the pixel width of a font string (sum of `fontCharWidths` entries,
  indexed by `c - 0x20` with `c >= 0x60` clamped to index 0x20), then submits it
  via `fontTextSubmit` centered on `x`: the text is drawn at `x - width/2`, and
  that centered left edge is returned.
- No boot-ELF caller (referenced only from overlays), so behavior is inferred
  from the body.
- `fontCharWidths` was `D_00189DC0`; named in `config/symbols.txt` (0x189DC0,
  the per-character glyph-advance table; entry i = width of char i+0x20).

## Dead tail at 0x1F0C48

`addiu sp,sp,0x30; nop` — a 0x30 stack deallocation matching no frame.
`fontTextSubmit`'s own 0x10 epilogue is the `addiu sp,sp,16` in its `jr ra`
delay slot at 0x1F0C40. EGC 2.95.2 never regenerates a dead frame dealloc after
the epilogue, so the 8 bytes are preserved with raw asm (glabel
`func_001F0C48`), mirroring the `func_001F0BC8` pattern in the same file. The
`.align 3` in that block also reproduces the alignment nop at 0x1F0C44.

## Replacement

```c
int fontTextSubmitCentered(int x, int y, int field_0x08, unsigned char* text) {
    int sum = 0;
    unsigned char* p = text;
    if (*text) {
        do {
            unsigned char c = *p++ - 0x20;
            int w = c;
            if (c >= 0x60)
                w = 0x20;
            sum += fontCharWidths[w];
        } while (*p);
    }
    x -= sum >> 1;
    fontTextSubmit(x, y, field_0x08, (char*)text);
    return x;
}
```

draw.o is a GNU-assembler TU (`-Wa,-EL -Wa,-Icode/include`, no private flags);
the probe was run with `--assembler gnu` and default flags to match.

## Codegen facts that matter (EGC 2.95.2 tiebreaks)

- **Parameter must be `unsigned char*`**: the guard `if (*text)` reads a3 and
  the `while (*p)` reads `*p`; both must be `lbu`. A `char*` param (or a cast
  on a boolean test, `if ((unsigned char)*text)`) keeps them `lb`.
- **`p = text` declared BEFORE the `if`**: puts `move t0,a3` in the `beqz`
  delay slot and keeps `move t1,zero` (sum=0) before the branch. Declaring `p`
  inside the `if` breaks the prologue order (11-word diff).
- **Width select must be `int w = c; if (c >= 0x60) w = 0x20;`** (preload the
  register value, then conditionally overwrite with the constant). This is the
  only form that emits the original: hoisted `li t3,32` (0x1F0C78), per-iter
  `move v1,t3` (0x1F0C84), `sltiu v0,a0,96` (0x1F0C98), `movn v1,a0,v0`
  (0x1F0C9C). Every alternative — `(c<0x60)?c:0x20`, `w=0x20; if(c<0x60) w=c`,
  `(c>=0x60)?c:0x20` — emits `sltu`/`movz` with the 32 inlined (no hoist).
- **`x -= sum >> 1;`** gives a single `sra v0,t1,1` (0x1F0CB4); `sum / 2`
  instead emits a 3-instruction `srl/addu/sra` rounding idiom.
- `c = *p++ - 0x20` (unsigned char) gives `lbu a0,0(t0); addiu a0,a0,-32;
  andi a0,a0,0xff` (the truncation).
- Table base is hoisted out of the loop into t2: `lui v0,%hi; addiu
  t2,v0,%lo` (0x1F0C74/7C), absolute (0x189DC0 is outside the GP window).
- Register map (fell out naturally): x→s0, y→t4, sum→t1, p→t0, table→t2,
  32→t3, c→a0, next→a1, width-idx→v1, guard→v0.

## Verification

Full clean build + `cmp build/boot_elf.elf assets/boot_elf.elf` → byte-identical.
The nop at 0x1F0CDC is the `.align 3` padding of the following `func_001F0CE0`
INCLUDE_ASM, not part of this function.
