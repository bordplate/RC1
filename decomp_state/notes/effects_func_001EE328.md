# func_001EE328 (real entry 0x1EE338) — BLOCKED

Source: `code/game/effects.cpp`, `INCLUDE_ASM(..., func_001EE328)`.
Splat symbol `func_001EE328` spans 0x17C bytes (380B) from 0x1EE328; the first
16 bytes (0x1EE328–0x1EE337) are a dead-tail of `func_001EE008`. The real
function `func_001EE338` (C linkage, called by `DrawDebugProfiler` at 0x1F3DF8)
is 364 bytes (91 words), 0x1EE338–0x1EE4A3.

## Semantics (verified, corrected 2026-09-26)
- If `D_001413D4==0x72`: `D_00189300.count=0`.
- If `D_00189300.count==0`: return.
- `ovp = occlViewParams+4` (0x13E510). `def.x = func_001FA6C0(ovp[2])`;
  `def.y = func_001FA6C0(ovp[3])`. `def.x` is the FIRST call's result — the
  delay-slot `swc1 $f0,0(sp)` at 0x1EE3A8 runs before the second jal.
- If `count<=0`: goto end.
- Loop `i in [0,count)`: `p = &sprites[i]` (stride 0x30).
  - `object = p->object` (a POINTER at p+0x20, NOT a state int).
  - Skip if `object==NULL`, or `((u8*)object)[0x20]==0xFE`, or
    `((u8*)object)[0x20]==0xFD`. The three `beql` skip-edges each carry a
    `lw $2,0xC0(base)` in the taken-delay slot purely to feed the
    `i<count` loop-bound check — the byte is at `object+0x20`, NOT
    `count+0x20`.
  - If `p->field_24!=0`: `projectWorldPoint(&pt,p)`;
    `pt.x=(pt.x-FA6C0(ovp[4]))*0.0625f`; `pt.y=(pt.y-FA6C0(ovp[5]))*0.0625f`.
    Else `pt=def` (128-bit `lq/sq` copy).
  - `func_001EE008(pt.x,pt.y,p)`.
- end: `D_00189300.count=0`.

## Best results (standalone probes, project flags)
- probe20 (old `state`-int semantics, pins i/$17 + ptp/$20, ovp normal):
  **356B, 67 word-diffs** — fewest diffs, but the three loop skip-tests emit
  `beqz` where the original has `beql` (wrong dataflow: kept `count` live
  through the condition chain).
- probe22 (corrected `object`-pointer semantics, same pins): **356B, 72
  word-diffs** — the three skip-tests now emit `beql` with the count loads in
  the taken-delay slots (correct). This is the accurate form.

## Residual (register allocation only)
The loop body + all three `beql` delay slots match modulo the s-register
permutation. Final allocation:
- Original: `p=s0, i=s1, &ovp=s2, base=s3, &pt=s4, %hi(0x189300)=s5, 0xFD=s6,
  0xFE=s7`. Initial: `base=s2, i=s1, &ovp=s0`, then a 3-`daddu` rotation
  (`s3=base(s2)`, `s2=&ovp(s0)`, `s0=p(s3)`) at 0x1EE3B8/C4/D4.
- EGC 2.95.2 (probe22): `p=s0, i=s1, &ovp=s2, &pt=s4, 0xFD=s5, 0xFE=s6,
  base=s7`, `%hi=s3`. Initial `base=s0, i=s1, &ovp=s2`; only ONE move
  (`s3`/`s7=base`), no rotation. Hence the 356-vs-364 (2-word) size gap.

Tried (all parity-safe, none closed it): no pins; i-only; i+ptp; i+ptp+ovp;
pin 0xFE→$23/0xFD→$22 (moved base to s5, %hi stayed s3 — swapped vs original);
declare `base` as a pointer pinned to $19 (count access then used s3 not s2,
83 diffs); `base` pointer unpinned (base→s2 initial but ovp→s3, no rotation,
81 diffs); source reorder (base/i/ovp); nested-if shape (no gotos, 72);
`-fno-schedule-insns` (360, 71), `-fno-schedule-insns2` (360, 83).

## Escalation
- `last-resort-decompiler` (GPT-5.6 Sol) invoked 2026-09-26: identified the
  real semantic bug (the p+0x20 field is a pointer `object`, byte at
  `object+0x20`, NOT `count+0x20`; the delay-slot count loads feed the loop
  bound) and prescribed the corrected RTL + pin matrix. Applied and
  mechanically diffed: the `beql`/delay-slot dataflow now matches, but the
  s-register permutation (base s7 vs s3; %hi s3 vs s5; 0xFE/0xFD s6/s5 vs
  s7/s6; missing 3-`daddu` rotation) is a register-allocator difference not
  controllable from C source. Confirmed the same class of blocker as
  camera_func_001EBF10.
- Retain `INCLUDE_ASM`; full-ELF parity preserved.
