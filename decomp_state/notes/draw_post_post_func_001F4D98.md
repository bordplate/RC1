# func_001F4D98 (0x1F4D98, 0x220 = 136 words) — BLOCKED

File: `code/game/draw_post_post.cpp` (INCLUDE_ASM at line 543). Blocked 2026-09-30.
Retained as `INCLUDE_ASM`; full boot-ELF parity preserved.

## Semantics
Occlusion-sample VU1 chain state machine. Appends one 0x80 (128-byte) VU1 packet
to `vu1ChainHead` (0x160F00) and advances it H0 -> H8. Control flow:

```
if (occlChainActive) { if (occlChainFrames < 0x18) occlChainFrames++; }
else                 { if (occlChainFrames == 0) return; occlChainFrames--; }
frames = occlChainFrames;
if (frames == 0) return;
frames <<= 4;                       // f = frames*16, in t1/$9
... build 5 records at H0..H7 ...
vu1ChainHead = (u32*)vu1ChainHead + 0x10;   // H4 -> H8 (fresh volatile reload)
```

## Globals (all GP-window; mixed address modes)
- `occlChainActive` = 0x15F444 (`int`); `occlChainFrames` = 0x15F448 (`int`).
  `occlChainFrames` reads are GPREL automatically in the two gate `beqz` delay
  slots (ps2eeas expands the bare pseudo GPREL16 inside the noreorder branch
  block) and self-based `lui/lw` elsewhere -> a plain `extern int` (no alias).
- `occlViewParams` = 0x13E500 (struct `OcclViewParams`: minX +0x10, minY +0x14,
  maxX +0x18, maxY +0x1C). MUST be volatile (or a volatile pointer) so every
  perspective word re-loads its coordinate fresh; caching A/B/C/D in locals makes
  EGC batch them (wrong).
- `vu1ChainHead` = 0x160F00: plain seeded alias `vu1ChainHeadGp` (`.extern
  vu1ChainHeadGp, 8`) for the single-instruction GPREL FIRST read that lands in
  the gate delay slot; double-volatile `extern volatile u32* volatile
  vu1ChainHead;` for every self-based re-read/store (the 2026-09-13 double-
  volatile idiom, see vuchain.cpp).
- Perspective 128-bit templates: A = 0x160820, B = 0x160830 (still UNNAMED in
  symbols.txt; the probe passes them as `--define`).

## Packet layout (5 x 16-byte records at H0..H7)
- Rec1 [H0]: `[0x10000007, 0, 0, 0x50000007]`. Each of the 4 stores re-reads the
  volatile head (fresh `lui/lw` per store); the FIRST re-read is the GPREL delay-
  slot load.
- Rec2 [H1]: 16-byte copy of template A, then word0 low 16 bits <- 0x8001
  (`sw store[3]` where store[3] = 0x8001 | (word0 & 0xFFFF0000)).
- Rec3 [H2]: `[0x104, 0x80000000]` as two `sd` (u64 constants).
- Rec4 [H3]: 16-byte copy of template B, then word0 low 16 bits <- 0x8008.
- Rec5 [H4..H7]: 8 x 64-bit perspective words. Each is TWO `or`s:
  `W = (X | (Y << 16)) | K` where X in {minX,maxX}, Y in {minY,maxY,minY+f,
  maxY-f}, and
  **K = 0xFFFFF300000000** built as `ori 0xFFFF; dsll 16; ori 0xF300; dsll 24` in
  a3/$7. The 8 words:
  W0=minX|(minY<<16)|K, W1=minX|((minY+f)<<16)|K, W2=maxX|(minY<<16)|K,
  W3=maxX|((minY+f)<<16)|K, W4=maxX|(maxY<<16)|K, W5=maxX|((maxY-f)<<16)|K,
  W6=minX|(maxY<<16)|K, W7=minX|((maxY-f)<<16)|K.

## Verified idioms
- 128-bit copies must be DIRECT: `*(OcclQuad*)dst = *(OcclQuad*)src;` with
  `OcclQuad = unsigned int __attribute__((mode(TI)))`. A 128-bit LOCAL (`OcclQuad
  q = ...`) forces a 0x10 frame the original lacks (candidate4 grew to 580 B).
- cfront (EGC 2.95.2) is C89: `register X asm("$N")` decls must be at the TOP of
  a block, not after statements mid-function-body; scoped `{ register ...; }`
  blocks work. Use a typedef to avoid `volatile struct` in a register decl.
- Splat `.s` quirk: the third comment field is on-disk bytes as big-endian hex =
  the little-endian instruction word REVERSED; byte-reverse before objdump
  comparison.

## Perspective-block register map (reproduced by candidate6/7)
a1/$5 = occlViewParams base, a3/$7 = K, a0/$4 = head re-read, a2/$6 = p = H4 base
(p = head + 4 u32), t1/$9 = frames/f.

## Match status
Best verified candidate (candidate7.c, scoped pins + last-resort fixes) compiles
to **548 bytes (137 words), 104-105 differing words** with production default
flags (`-G8 -O2 -ffast-math -fno-exceptions -snas`); `-fno-schedule-insns` = 108,
`-fno-schedule-insns2` = 107 (both WORSE). candidate5 (direct copies, no scoped
loads) was 540 B / 115. The gate/frames prologue (words 0-22) and the tail match
in candidate5/7; the residual is entirely in the packet-build body.

## Residual (why it does not match)
A sorted-multiset comparison of raw instruction words (candidate7a, 137 words vs
original 136) shows the instruction multisets are NOT identical — so this is not
pure scheduling. Three coupled EGC 2.95.2 RA/scheduler walls in the body:

1. Per-word coordinate register allocation. The original loads each perspective
   word's X/Y coordinates into short-lived $2/$3 (and $4 for one word) and ORs
   the result into the X register; the candidate allocates the same loads to a
   different register set (e.g. maxY->t1/$2 and maxX->a0/$4 where the original
   uses $3/$2). The expert's "short-lived register" pins (perspective-scope
   view/K/base/p/frames) fixed the persistent registers but EGC still picks
   different registers for the 16 per-word coordinate loads, and the original's
   exact (X-reg, Y-reg) pair per word cannot be forced without 16 conflicting
   hard pins that EGC's allocator re-coalesces.
2. Scheduling. Even where the registers agree, the per-word load/or/sd order and
   the head re-read placement differ (the "same instructions, wrong order" class);
   both scheduler flags make it worse, so the original used the default scheduler
   and no TU flag resolves it.
3. K-build form. `K |= 0xF300` on a u64 is a 64-bit OR that EGC lowers to
   `li v0,0xf300; or a3,a3,v0` (2 instrs), while the original uses the 32-bit
   `ori a3,a3,0xf300` (1 instr). This accounts for the 548-vs-544 (1-word) size
   gap. Removing the anti-folding barrier does not change the form (EGC still
   uses the register OR); keeping all barriers is required to stop EGC folding the
   whole `ori;dsll;ori;dsll` into a single `dli`.

## Variants tested
candidate1-3 (plain/volatile mixes): 408-476 B, 119-122 diffs. candidate4 (128-bit
`q` local): 580 B (forced frame). candidate5 (direct 128-bit copies, function-wide
pins frames->$9/K->$7/base->$4/p->$6, K via 4 statements + barriers): 540 B / 115
(words 0-22 + tail match). candidate6 (expert's scoped pins + clobber frontier):
540 B / 111 (perspective persistent registers correct). candidate7 (last-resort:
final-head fresh reload, drop post-0xF300 K barrier, scoped Y-then-X volatile
loads): 548 B / 104. candidate7a (steps 1-2 only, direct expressions): 548 B / 105.
Flags `-fno-schedule-insns` / `-fno-schedule-insns2` on candidate5 and candidate7a:
worse. Register pins, barriers, `.extern`-seeded aliases, double-volatile head,
plain GPREL alias all exercised.

## Escalation
- decomp-researcher: thorough report; corrected K to 0xFFFFF300000000, required
  volatile occlViewParams, dropped the unnecessary occlChainFrames Gp alias, and
  specified the vu1ChainHeadGp plain seeded alias for the first GPREL read.
- expert (GPT-6 Astra): "not a version wall — a register-lifetime/RTL-shape
  problem"; prescribed scoped short-lived hard-register pins + a clobber frontier
  + correct constant types (S16 patches, U64 sd constants). Implemented ->
  candidate6 (111).
- last-resort-decompiler (GPT-5.6 Sol): verdict do NOT block yet; 3 concrete fixes
  — (1) final advance must be a fresh volatile reload
  `vu1ChainHead = (u32*)vu1ChainHead + 0x10` (not `(u32*)p + 8`, which emitted an
  extra `addiu t0,a0,48`); (2) remove ONLY the barrier after `K |= 0xF300` to
  restore inline `ori`; (3) sequence volatile field loads Y-first-then-X via scoped
  statement-form reads. All three implemented -> candidate7 (104); (2) did NOT
  restore `ori` (EGC still uses the register OR), (3) added the scoped loads.

## Blocker
EGC 2.95.2 (default scheduler) cannot reproduce the packet-build body of
func_001F4D98 while keeping the already-exact gate/frames prologue and tail: the
per-word coordinate register allocation in the 8 perspective words (different
(X,Y) register pair per word, not forceable without 16 conflicting hard pins),
the per-word load/or/sd scheduling, and the u64 `K |= 0xF300` lowering
(`li+or` vs `ori`, the 1-word size gap). No source form, register pin, barrier,
or tested TU flag resolves these without regressing an already-matching region.
last-resort GPT-5.6 Sol used (fixes implemented, residual confirmed). Retain
`INCLUDE_ASM`; full boot-ELF parity preserved. Re-attempt starting point:
candidate7.c (scoped view/K/base/p/frames pins + double-volatile head + GPREL
alias + fresh-reload tail) — residuals are the three walls above.
