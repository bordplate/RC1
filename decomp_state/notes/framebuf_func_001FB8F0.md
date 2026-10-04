# func_001FB8F0 (0x1FB8F0, 0x1C0 = 448 bytes) — BLOCKED (2026-10-04)

AA-pass **inline GS display-environment** packet builder. Blocked on EGC 2.95.2
q-region register-allocation / instruction-scheduler tie-breaks after the
prologue + arithmetic were SOLVED (via OR-operand reversal from the expert, and
the void-return correction from the last-resort). Best candidate = 448 bytes /
40 word-diffs (~91% match); prologue, hi-pair arithmetic, and w0/w1 build all
byte-correct.

## What it does

`framebuf_appendAAEnv(u32 p1..p7) -> void` (void; the new head the callers read
from `v0` is left there incidentally by the trailing `func_00233938` call).
Appends, to the `vu1ChainHead` VU1 command chain:

1. A `func_00233938` (append-data-ref-record) call with arg `0x13000000`, then
   `VU1_addGSregister(0x42, 0x64)`.
2. A 4-word record at the head: `[0x10000006, 0, 0, 0x50000006]`.
3. An **inline GS display-environment** payload of 12 qwords at head+0x10:
   q0=0x1000000000000001, q1=0xE, q2=0x33003, q3=0x47, q4=0x2400000000000001,
   q5=0x10, q6=0x106, q7=p7 (zero-extended), q8=0x2400000000008001, q9=0x44,
   q10=w0, q11=w1 where:
     hi0=(p2*0x10+0x8000)-p6*8, hi1=(p4*0x10+0x8000)-p6*8,
     lo0=(p1*0x10+0x8000)-p5*8, lo1=(p3*0x10+0x8000)-p5*8,
     w0 = lo0 | (hi0<<16), w1 = lo1 | (hi1<<16).
4. Advance the head to head+0x10 (GPREL store), then a tail: load head, +0x60,
   GPREL-store to `vu1ChainHead` (in the GS `jal` delay slot),
   `VU1_addGSregister(0x42, 0x8000000044)`, trailing `func_00233938(0x13000000)`.

This is the runtime twin of the static AA display-env blocks (see the matched
`framebuf_appendSmallSetup` / `framebuf_appendClearBlackDataRef` and
`framebuf_SetupFS_AA_buffer__Fiiiiii`); the payload resembles Deadlocked's
`aaMovieDisplayRegs` / `SetMovieDrawSmallVIFChain` GS display environment.
Callers: the vendor 6-pass AA loop (0x238xxx–0x239xxx, 9+ sites) — p5=0x200,
p6=0x80 at all sites.

## Best probe (40 word-diffs, exact size 448 B)

`working/framebuf_func_001FB8F0/var_L1_void.cpp` via `tools/decomp_probe.py`,
symbol `framebuf_appendAAEnv__FUiUiUiUiUiUiUi`,
`--define vu1ChainHeadStore=0x160F00 --define func_00233938=0x233938 --define vu1ChainHead=0x160F00`.

Key source forms that were REQUIRED to get here:
- **OR operands low-first**: `long w0 = lo0 | ((long)hi0 << 16);` (or the
  accumulator form `long w0=lo0; w0|=(long)hi0<<16;`). This flips EGC's
  prologue allocation to the correct `s1=a3(p4), s0=a1(p2)` and moves the
  packed results into `s3(w0)/s2(w1)`. (Discovery: expert GPT-6 Astra.)
- **Arithmetic orders**: decl `hi0,hi1,lo0,lo1`; `+=`/`-=` `hi1,hi0,lo0,lo1`;
  `w0` built before `w1`.
- **void return** (not u32), with `vu1AppendDataRefRecord` still declared
  `u32` (non-void callee). (Discovery: last-resort GPT-5.6 Sol.)

Everything matches EXCEPT the residual 40 words below.

### Residual 40 diffs

**A. lo-pair `+=`/`-=` emit order (4 words: 0x1FB980/84, 0x1FB9AC/B8).**
Original `addu`/`subu` order is s1,p4; s0,p2; **s3,p1; s2,p3** (hi1,hi0,lo0,lo1).
L1 emits s1,p4; s0,p2; **s2,p3; s3,p1** (hi1,hi0,lo1,lo0). The register
assignment is identical (s2=p3, s3=p1 in both) and the source writes lo0 before
lo1, yet EGC's default machine scheduler emits the two independent,
same-latency lo `addu`s/`subu`s in the opposite order. The hi pair (s1 then s0)
follows source order fine; only the lo pair is re-scheduled. Confirmed
uncontrollable: an explicit-pseudo form (`biasedLo0=0x8000+lo0; lo0=biasedLo0;`)
does not change it.

**B. q-packet region (~36 words: 0x1FB9CC–0x1FBA68).** The 12 qword stores and
their 64-bit-constant materializations. Three coupled differences:
1. **q0 const register**: L1 puts 0x1000000000000001 in `v0`; the original in `a0`.
2. **L5 old-head register**: L1 loads `vu1ChainHead` into `v1` and advances it
   **in place** (`addiu v1,v1,16`) to the new head, storing q0 via `sd v0(q0),16(v1)`;
   the original keeps **two separate** live registers — old head in `v0` (q0 via
   `sd a0(q0),16(v0)`) and new head in `v1` — so the advance does not overwrite the
   old-head base still used by the q0 store.
3. **64-bit q-constant register assignment + store interleaving** differ
   (q3 a2-vs-t0, q4 a3-vs-a2, q5 t0-vs-v0, etc.).

The original's extra simultaneous live range (old-head + new-head + q0-const
across the advance store) changes the interference graph, which re-colors every
q-constant register and re-schedules the stores. This is the same
old/new-head-base-split class as func_001FB740 (sibling, also blocked).

## What was tried (all full-function probes; best 40)

- Declaration-order permutations of hi0/hi1/lo0/lo1 (8+ orders): never flips the prologue.
- `+=`/`-=` statement orders incl. the exact original order: lo pair re-scheduled regardless.
- w0/w1 build order: w0-before-w1 required.
- OR-operand reversal / accumulator form: **fixes prologue** (V2/V3=47, basis of V5).
- void return (L1): 40 (best). u32 return (R3): 45.
- Inline w-expressions (no temporaries): 460 B, 88-91.
- Separate old-head local + `oldHead[2]=q0const` before forming advanced pointer (V1/V6/L2): canonicalized, 52-58.
- `u32* oldHead/newHead` two-pointer form (L3): 43.
- Explicit q-const pseudos (L4): 43.
- q-store source order natural vs original: 65.
- Advance-store source position early vs late: late=40 (best), early=61.
- Register pins on any of hi0/hi1/lo0/lo1: 73-82 (pins hoist the dslls before the 2nd call).
- Scheduler flags `-fno-schedule-insns` / `-fno-schedule-insns2`: 65-99 (worse).
- Parameter-update form (`p2<<=4; p2+=...`): 76.
- lo-pair explicit-pseudo form (L5): 40 (no change).

## Escalations

- **expert (GPT-6 Astra)** — 4 variants; the OR-operand-reversal / accumulator
  recommendation is what flipped the prologue allocation (the key win).
- **last-resort-decompiler (GPT-5.6 Sol)** — corrected the return type to void
  (Ghidra marks both this fn and func_00233938 void; callers read the incidental
  v0), giving 41->40. Prescribed 5 variants (void-target; explicit old/new bases
  advance-before-q0; u32 two-pointer; explicit q-const pseudos; lo-pair pseudos) —
  all applied and mechanically diffed; none closed the q-region/lo-pair residual.
  Concluded the old/new-head allocation (one incremented pointer pseudo vs the
  original's three simultaneous live values) is the strongest blocker, and the
  lo-pair reorder has "no credible strictly-C lever left" in this build.

Retain `INCLUDE_ASM`; full-ELF parity preserved. Re-attempt start point:
`working/framebuf_func_001FB8F0/var_L1_void.cpp` (void target, OR-reversed w0/w1,
correct arithmetic orders). Any new attempt must keep the void + OR-reversed
forms or it regresses to 45+.
