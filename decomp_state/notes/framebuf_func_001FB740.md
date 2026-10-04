# func_001FB740 (0x1FB740, 0x1B0 bytes) — BLOCKED (2026-10-04)

AA-pass **inline** VU1 program builder. Blocked on two EGC 2.95.2 scheduler
tie-breaks after the decompiler-recommended source reshaping reduced the
residual to 12 word-diffs at the exact right size.

## What it does

Appends an *inline* VU1 program record (VIF code `0x10`, tag `0x10000000`,
end tag `0x50000000`) to the VU1 command chain for the AA post-process.
`count = (w > -1 ? w : w + 0x1F) >> 5` (ceil-ish `w/32`); `qcnt = count + 5`.

1. Tag block at the head: `[0x10000000|qcnt, 0, 0, 0x50000000|qcnt]`.
2. Advance the head +4 words via the GPREL `vu1ChainHeadStore` alias.
3. Write 10 fixed 64-bit payload words (a VU program header):
   W0=0x1000000000000001 (stored from the **raw** head base, `sd $13,0x10(head)`),
   W1=0xE, W2=0x32003, W3=0x47, W4=0x2400000000000001, W5=0x10, W6=0x146,
   W7=0x80008080, W8=((0x9000<<46)|0x8000)|count (runtime, `or $5,$2,$5`),
   W9=0x44 (W1..W9 on the `head+0x10` base).
4. do-while of `count` iterations, two 64-bit row-copy words each:
   word0 = ((0x8000 - 8*w + 0x200*i)&0xFFFF) | (((0x8000 - 8*h + 0x200*i)&0xFFFF)<<16)
   word1 = ((0x8200 - 8*w + 0x200*i)&0xFFFF) | (((0x7FF0 + 8*h + 0x200*i)&0xFFFF)<<16)
5. Advance the head `count*4 + 0x14` words (GPREL).

Called **twice per AA sub-pass** by the vendor 6-pass loop `func_00239780`
(0x239780, vendor.cpp) with **(0x200, 0x200)** at 0x239B14 and 0x239BDC.
It is the dynamic twin of the static AA VU-program blocks built by
`SetupFS_AA_buffer__Fiiiiii` (0x1FA978) — the static siblings are streamed by
the matched `AA_BlurPass`/`framebuf_appendSmallSetup` data-ref appenders.
Proposed name: `AA_appendInlineProgram` (mangles `AA_appendInlineProgram__Fii`).

## Best probe (12 word-diffs, exact size 432 B)

`working/framebuf_func_001FB740/probe_best.cpp` (via `tools/decomp_probe.py`,
symbol `func_001FB740__Fii`, `--define vu1ChainHeadStore=0x00160F00`).
Everything matches **except** 12 words in two scheduler tie-breaks:

- two-base 64-bit stores (W0 raw head base vs W1-W9 head+0x10 base) — **MATCH**
- 64-bit constant materialization (ori/dsll32-or-dsll/ori 3-instr) — **MATCH**
- payload register allocation (W0=$13,W1=$11,W2=$12,W3=$8,W4=$9,W5=$10,
  W6=$6,W7=$7,W8=$5,W9=$3,counter=$14) — **MATCH**
- payload store region (0x1FB824..0x1FB85C) — **MATCH**
- W8 construction `((count|0x8000)|(0x9000<<46))` — **MATCH**
- loop-counter placement (nonvolatile `$14` transfer just before the `if`) — **MATCH**
- **entire loop body** (0x1FB890..0x1FB8B8) — **MATCH**

### Residual 12 diffs

**A. 0x1FB85C..0x1FB88C (loop setup, 9 diffs).** The original reuses `$5`/a1
for BOTH the `0x8000` constant (`li a1,0x8000`) AND the body base
(`addiu a1,t7,96`) at different times. The required
`register u64* body asm("$5")` pin (needed to keep the loop body exact) locks
$5 for the body base, so EGC spills the 0x8000 constant to $6 and permutes the
setup. Removing the body pin fixes the setup but breaks the loop body
(net worse: 22 diffs).

**B. 0x1FB8BC..0x1FB8D4 (tail, 3 diffs).** EGC orders the advance
`sll count,4; addiu 0x50` before/around the head `lui/lw` and the `lq ra` /
`lq s2` restores differently than the original's
`lui, lw, sll, addiu, lq ra, addu, lq s2, lq s1, lq s0, lui, sw`. A barrier
after the head reload swaps the addiu/lq-ra pair (13 diffs, worse).

### Ruled out

- W8 order `((0x9000<<46)|0x8000)|count` vs `((count|0x8000)|(0x9000<<46))`
  (the latter matches).
- 64-bit constants as full literals vs `(HIGH16<<SHIFT)|LOW` (identical;
  both expand to the correct 3-instr sequences).
- `w*8`/`h*8` computed once (shared, REQUIRED for the right size) vs twice.
- Pinning W9 to $3 / loop counter to $14 directly (perturbs the allocator,
  80 diffs).
- Body pin removed (22 diffs, loop body breaks).
- Tail barrier (13 diffs, swaps addiu/lq-ra).
- TU flags: -fno-schedule-insns 84, -fno-schedule-insns2 66,
  -mno-split-addresses 44 (all worse than default 12).

## Last-resort

last-resort GPT-5.6 Sol (2026-10-04) confirmed the residual is three
scheduling regions (the `$14` init placement, the loop-setup permutation, the
tail order) — **not** the 64-bit constants or two-base stores — and returned
the source reshaping that took the probe from 66 to 41 diffs (loop body exact).
Applying it plus the W8-order fix and the moved `$14` transfer reached 12
diffs. The two remaining tie-breaks (loop-setup $5 reuse, tail order) could
not be matched by any further source/pin/barrier form.

Retain `INCLUDE_ASM`; full-ELF parity preserved.
