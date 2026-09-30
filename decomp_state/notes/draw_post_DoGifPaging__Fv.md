# DoGifPaging__Fv (0x1F4398, 0x11C = 71 words) — BLOCKED

File: `code/game/draw_post_post.cpp`. Blocked 2026-09-29. Retained as
`INCLUDE_ASM`; full boot-ELF parity preserved.

## Semantics
Fills the two 16-byte GIF-page VU1 data-reference records that
`SetupGifPaging__Fi` reserved (`gifPageMarkerA`, `gifPageMarkerB`): each record
is written `[0]=0x20000000` (GIF page tag), `[1]=(u32)chain link`,
`[2]=[3]=0`, and `vu1ChainHead` is advanced 16 bytes (one record) after each.
Between the two records, if `drawTextureDmaState[11] != 0` it rebuilds the GIF
texture DMA (`func_0020B4A8`) and flushes the VU1 texture pipeline
(`VU1_texFlush__Fv`), then re-reads the chain head (the branch phi feeds
`head[0]=tag`). Deadlocked reference: `X:\rcb\code\stable\game\draw.cpp`
`DoGifPaging` (calls `BuildGifTextureDma()` with NO argument).

## Signature correction (useful for other draw_post targets)
`func_0020B4A8` takes **no arguments**: `extern "C" void func_0020B4A8();`.
Ghidra types it `FUN_0020b4a8(void)` and Deadlocked calls it arg-less. Its
first apparent use of a0 (`srl t1,a0,0x8` at 0x20b4bc) is DEAD — `t1` is
overwritten by `lhu t1,6(a3)` at 0x20b4e4 before any use. So the `a0` present at
the `jal 0x20b4a8` (0x1f440c) in the original is merely residue from the
preceding `a[1]` marker store (gifPageMarkerA in a0), not a call argument.

## Key mechanism (the hard part — SOLVED)
The branch delay-slot load at 0x1f4408 is a **GPREL** 1-instr read of
`vu1ChainHead` (`lw r,-23808(gp)`), while every other `vu1ChainHead` read in the
function is an **absolute** 2-instr `lui r,0x16; lw r,3840(r)`. ps2eeas expands
a bare `lw r, sym` as GPREL16 only when the reference sits inside a `.set
noreorder` region (EGC wraps branch/jal blocks in noreorder) or after an
`.extern sym,N`. A load that lands in the `beqz` delay slot is inside that
noreorder region, so it expands GPREL with **no alias/seed needed**. A load that
lands outside the branch (the in-if reload, the second half) stays absolute.
EGC 2.95.2 will only MOVE a NON-VOLATILE load into the delay slot; a volatile
load is immovable. So the working C form is:

```c
u32* head2 = vu1ChainHeadPlain;   // plain (non-volatile) alias, same 0x160F00
if (flag != 0) { func_0020B4A8(); VU1_texFlush(); head2 = vu1ChainHead; }
head2[0] = tag;                    // head2 = phi(delay-slot GPREL load, in-if absolute)
```

with `extern u32* volatile vu1ChainHead;` and `extern u32* vu1ChainHeadPlain;`
(same address 0x160F00). This reproduces the GPREL delay-slot load + plain
`beqz` + single merged `sw s0,0(v1)` tag. A volatile pre-if load instead makes
EGC hoist it and emit `beqzl` with the TAG in the delay slot (a semantic bug:
spurious store to the stale head on the taken path) — do not use volatile there.

## Globals (all GP-window; absolute lui/lw except the one GPREL delay-slot read)
- `vu1ChainHead` = 0x160F00 (`u32*`); `vu1ChainHeadPlain` = same address, plain.
- `gifPageMarkerA` = 0x15F450, `gifPageMarkerB` = 0x15F454 (`u32* volatile`;
  each `marker[i]` re-reads the base — fresh lui/lw per subscript).
- `drawTextureDmaState` = array @ 0x18A2B0; `[11]` = 0x18A2DC, 2-reg split load
  `lui a1,0x19` (hoisted) + `lw v?, -23844(a1)`.
- `func_0020B4A8` @ 0x20B4A8 (no args), `VU1_texFlush__Fv` @ 0x233B68.
- `u32 tag = 0x20000000` local (the 0x20000000 lives in $s0 across the function).
- Frame 0x20: s0 @ 0(sp), ra @ 16(sp).

## Match status
Best verified candidate (v47.cpp) compiles to **284 bytes, 16 differing words**
with production default flags (`-G8 -O2 -ffast-math -fno-exceptions -snas`).
Everything from **0x1F442C through the return matches exactly** (the entire
second half, incl. the `(volatile u32*)vu1ChainHead`/`(volatile u32*)markerB`
cast form + block-scoped `$2/$3/$4` pins). The 16 residual words are four
coupled first-half / branch scheduling-RA conflicts:

1. 0x1F43B8–0x1F43CC (6): first-half order. Original
   `[A1 load][head+=16][head store][sq ra]`; candidate
   `[head+=16][sq ra][A1 load][head store]`. A barrier can move the A1 load
   before the add but also pulls `sq ra` ahead of it — no tested dependency
   yields both original orders.
2. 0x1F43E4–0x1F43F8 (6): EGC hoists each following `gifPageMarkerA` base load
   ahead of the preceding ordinary marker store (batches the A3/A4 loads before
   the `a[1]`/`A[2]` stores); the original interleaves load-then-store per
   record. Making even the first marker store `volatile` fixes this interleaving
   but flips the branch to `beql`, duplicates the merged tag store, and grows the
   function to 292 bytes (v45).
3. 0x1F4400/0x1F4404 (2): cond value register. Base is correctly pinned to a1
   (`register int* dma asm("$5") = drawTextureDmaState;`), but EGC picks `v0`
   for `drawTextureDmaState[11]` where the original uses `v1`. Pinning the cond
   to `v1` makes the branch correct but steals `v1` from the delay-slot
   `vu1ChainHeadPlain` load (it becomes `v0`). The original reuses `v1` because
   the cond dies at the branch while the delay-slot load defines the
   branch-result value; EGC's hard-register lifetime model does not coalesce that.
4. 0x1F4424/0x1F4428 (2): merge order. Original `[lq ra][sw s0,0(v1) tag]`;
   candidate `[sw s0,0(v1) tag][lq ra]`. No tested barrier/volatility reorders
   this without breaking cluster 1/2 or triggering the 292-byte beql form.

## Variants tested (reloc-masked 71-word diff vs assets/boot_elf.elf)
Primary (V5–V21): pointer-volatile globals + plain locals + `(u32)` casts is the
only form that restores the 0x20 frame / s0 tag slot (double-volatile → qualifier
errors/spills). if/else placements, pre-if/post-if head reload, `.extern`-seeded
aliases, barriers, `register asm("$N")` pins. Moving `gifPageMarkerA[0]=tag;`
BEFORE the `vu1ChainHead=head+4` advance consistently triggers `beqzl` + 8 extra
bytes — avoid that order. Best primary: V20 (separate first-half `head` + branch
`head2` + block-scoped pinned `a`) = 284B/55.
Last-resort (V40–V47): added the no-arg callee, the `dma` base pin to a1, the
`packetValue`/`markerA*` pins, and the second-half `(volatile u32*)` casts —
reached **284B/16** (v47).
Flags on V15/V19/V20/V47: `-fno-schedule-insns`, `-fno-schedule-insns2`,
`-mno-split-addresses`, and combos — none closed the 16; `-fno-schedule-insns2`
fixes the branch-region registers but breaks the prologue (L1 no longer hoisted
before the prologue, `sq ra` early). `-fno-if-conversion`/`2` are unsupported.

## Escalation
- last-resort-decompiler (GPT-5.6 Sol) invoked 2026-09-29 with the full dossier
  (all 71 words, the GPREL mechanism, V20 as baseline). It (a) corrected the
  `func_0020B4A8` signature to no-arg, (b) produced v47 (284B/16, second half
  exact), and (c) concluded the residual 16 are four coupled EGC 2.95.2
  default-scheduler / register-allocation tiebreaks in the first half + branch
  (clusters 1–4 above) that no source form, register pin, barrier, or tested TU
  flag resolves without breaking an already-matching cluster or changing the
  CFG/size.

## Blocker
EGC 2.95.2 (default scheduler) cannot reproduce, while keeping the already-exact
second half and the GPREL delay-slot branch: (a) the first-half
`[A1 load][advance][sq ra]` order and per-record load/store interleaving, (b) the
`v1` cond/branch-result register coalescing, and (c) the merge `lq ra`-before-tag
order. Any single control that fixes one cluster regresses another or triggers
the 292-byte beql form. Retain `INCLUDE_ASM`; full boot-ELF parity preserved.
Re-attempt starting point: v47.cpp structure (no-arg callee, a1 base pin,
`(volatile u32*)` second-half casts, block-scoped `$2/$3/$4` pins) — the 16
residuals are clusters 1–4 above.

## Re-confirmation (2026-10-01)
A full second independent pass (fresh candidate lineage, `.extern`-seeded
same-address alias for the delay-slot load instead of the plain-alias form,
u32 value locals for the subscript stores) reached 284B/17 with the SAME four
first-half/branch clusters — no new lever found, confirming the residual is
stable across candidate lineages. Escalations this pass: expert (GPT-6 Astra)
and last-resort-decompiler (GPT-5.6 Sol); the one new fix found (explicit
B-region volatile read order: both `vu1ChainHead` and `gifPageMarkerB` reads
before either store, matching the original's `[P8 load][B1 load][addiu][stores]`
order) is already covered by the v47 second-half cast form. No flag
(`-fno-schedule-insns[2]`, `-mno-split-addresses`, combos), register pin,
scoped transfer, barrier, callee-prototype variant, or local-type variant
closed any cluster. Do NOT re-select this target; see clusters 1–4.
