# func_001FA860 (0x1FA860) — BLOCKED (handwritten signed-asm "camp")

`code/game/framebuf.cpp`, C-linkage, 0xF8 (248) bytes. Verified name from the
Deadlocked debug symbols: **`FastMapMaskRLE`** — `tools/ccc/stdump functions
reference/SCUS_974.65 | grep FastMapMaskRLE` prints
`/* ffffffff 000000f8 */ FastMapMaskRLE() {}`, i.e. the identical-size
engine descendant. Rename deferred (function stays an INCLUDE_ASM placeholder;
apply `FastMapMaskRLE = 0x1fa860;` in the framebuf symbols section + `make
split` if desired — pure relabel, no compiled code references the symbol).

## What it does (confirmed from ground-truth objdump)

Strided-bit RLE decoder for a 1-bit mask image decoded from a shared 32KB bit
table at arbitrary bit offsets.

```c
int FastMapMaskRLE(s8 *out, s32 length, u8 *in, u8 *table)
```

- `in` is a stream of (delta, count) byte pairs: "advance `bitpos` by `delta`
  bits, then decode `count` bits". Walked until `bitpos` reaches the sentinel
  0x40000 (= 32KB × 8 = one past the end of the shared table). `in += 2` runs
  on EVERY pair (it sits in the normal-branch delay slot of the 0x1fa898 `bne`).
- `table` is the shared 32KB bit table (`DAT_001a00fc` in .bss, runtime-filled
  by level overlays). Bits read LSB-first: `(table[bitpos>>3] >> (bitpos&7)) & 1`.
- Output is alternating run lengths (first run capped at 127 via `maxrun=127`
  init, then 255). When the bit flips, the finished run is written and
  `run=1, maxrun=255`. While the bit repeats, `run++` (taken-delay slot of the
  `bnel`) until `run == maxrun`, at which point the run is flushed as the
  TWO-byte pair `(maxrun, 0)` (`*out=run; out+=2; out[-1]=0`) and `run=1`.
- Epilogue: write the in-progress `run` if nonzero, then
  `*base = (*base << 1) + 1` (signed `lb` then `sll`/`+1`/`sb`): packs the
  first run length into the tag byte `(r0 << 1) | 1` (format bit 1 = RLE).
  Returns `out - base`, or -1 if `out == end` after any increment.
- Caller wrapper `FUN_00207b08` (menu, `func_00207B08` in
  code/_generated/nonmatchings/game/menu/): if flag `DAT_001a0118` is 0 it
  `FastMemSet(buf,0,0x800)`, else `n = FastMapMaskRLE(buf,0x800,DAT_001a0104,
  DAT_001a00fc)`; on `n == -1` it falls back to `FUN_00208030(buf)` (the
  nibble-delta packed-image decoder, which tags the buffer `(x & 0xFE) | 2` =
  format 2), then records `max[D_0015ED84] = max(..., n)` in the 0x13D560
  table. Buffer = `DAT_0015ed84 * 0x800 + 0x141EC0` (a 0x800-byte image-pool
  entry). Wrapper is called from 0x20b1d0 and `FUN_002269c0` (0x226a20).
- Type evidence: `lbu` for `in[]`/`table[]` (unsigned char*), signed `lb` for
  the `*base` epilogue load (s8* out).

## Why it cannot match: the "signed camp"

The original uses **trapping signed R-type `add` (funct 0x20)** for register
moves AND all additions, and **`sub` (funct 0x22)** for subtractions:

```
1fa860: add  a1,a0,a1      1fa868: add  t0,zero,zero
1fa870: add  t3,zero,zero  1fa874: add  t6,a0,zero
1fa878: add  v0,zero,zero  1fa890: add  t0,t0,at
1fa8b0: add  t9,t9,a3      1fa8dc: add  t1,at,zero
```

No compiler emits trapping arithmetic; EGC 2.95.2 SN emits `move` (ps2eeas
expands the pseudo to funct 0x2D) for moves and `addu`/`daddu` (funct 0x25)
for additions. My compiled candidate's leading words are `0080402d move
t0,a0`, `01056021 addu t4,t0,a1` — 0x2D/0x25, not 0x20.

This function belongs to an isolated "signed camp" in the boot ELF:

- Full `.text` scan: exactly **9 add-moves (funct 0x20, rt=0)** in the whole
  image, in only 4 functions: `func_001FA860` (5: 0x1fa868/70/74/78/dc),
  0x1ee6c4 (cmecpu area), 0x2123c4+0x212bdc (mobyproc area), 0x234320
  (tfragproc area). Versus 5507 `move` (0x2D) and 650 `daddu` (0x25) words.
- All 4 camp functions use signed `add` for every addition (incl. pointer
  adds and moves) and `sub` for subtractions, and each carries exactly ONE
  branch-likely in ~28KB of camp code (this one's `bnel` at 0x1fa8f8) vs
  thousands of branch-likely elsewhere.
- **0 of the 319 already-matched functions contain any funct 0x20 word** —
  EGC has never emitted it in a matched case; the camp was compiled by a
  different build (or is handwritten — see below).
- Splat/spimdis flags this unit `/* Handwritten function */` (one of only 7
  nonmatching units so marked; 0 matched) — the marker fires on the trapping
  `add`/`sub` R-type words, which a compiler cannot generate.
- Compiler fingerprint tests (all FAIL to emit the camp style): local EGC
  2.95.2 SN with C and C++ at -O1/-O2/-O3 and -mips1/-mips2 (always
  `move`+`addu`); the 991111b game compiler rebuilt from source with Lombyte's
  26 patches (its `addsi3_internal` emits `addu` unconditionally — same
  result); assembler `move` expansions (ps2eeas→daddu 0x2D, project GNU
  as→or 0x29; the binary contains zero or-moves).
- Secondary irreproducible details (would each alone produce diffs even with
  perfect RA): the SAME-path `bnel t3,v1,TOP` polarity — EGC emits
  branch-likely here but only inverted (`beql` to the flush) for every loop
  form tried; a DEAD `sub at,a1,a0` at 0x1fa908 (value unused, no branch
  target) that EGC's DCE removes; and four empty NOP scheduling holes
  (0x1fa8c0/c4/cc/d4) EGC always fills.
- Cross-project: Lombyte left all three sibling camp functions and this one as
  INCLUDE_ASM stubs.

Conclusion: the original is a handwritten signed-arithmetic assembly routine
(trapping add/sub, conservative scheduler). Byte-identical output from C is
impossible with any compiler/assembler/flag in this repository.

## Best C candidate (semantics verified; 228 B, 62/62 word diffs, all from
word 0 — the whole layout differs because of the camp codegen)

```c
int func_001FA860(s8 *out, s32 length, s8 *in, s8 *table) {
    s8 *end = out + length;
    s32 maxrun = 127;
    s32 bitpos = 0;
    s32 prev = 1;
    s32 run = 0;
    s8 *base = out;
    s32 remaining = 0;
    do {
        while (remaining != 0) {
            s32 bit = (table[bitpos >> 3] >> (bitpos & 7)) & 1;
            bitpos++;
            remaining--;
            if (prev == bit) {
                if (run != maxrun) {
                    run++;
                    continue;
                }
                *out = run;
                out += 2;
                run = 1;
                if (out == end) return -1;
                maxrun = 255;
                out[-1] = 0;
            } else {
                prev = bit;
                *out = run;
                out++;
                if (out == end) return -1;
                maxrun = 255;
                run = 1;
            }
        }
        bitpos += in[0];
        remaining = in[1];
        in += 2;
    } while (bitpos != 0x40000);
    if (run != 0) {
        *out = run;
        out++;
    }
    *base = (*base << 1) + 1;
    return out - base;
}
```

(Refine `in`/`table` to `u8*` for the `lbu` loads if the camp compiler is
ever identified; the loop skeleton above is the exact encoded structure. If
the function is ever hand-assembled for byte preservation, keep this C as the
semantic reference.)

## Last-resort escalation

`last-resort-decompiler` (GPT-5.6 Sol) invoked 2026-10-03 with the full
dossier (ground truth, camp scans, fingerprint tests, irreproducible
details). Recommendation: **record a blocker** — "Original is a handwritten
signed-arithmetic assembly routine; available compilers emit addu/daddu and
optimize away its dead/scheduling artifacts, so byte-identical output is
impossible from C without recreating the whole function in inline assembly."
No concrete C/flag/assembler recommendation was returned; none was testable.
