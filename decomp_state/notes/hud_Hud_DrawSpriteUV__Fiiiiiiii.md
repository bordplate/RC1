# Hud_DrawSpriteUV__Fiiiiiiii (func_00200258, 0x210 = 528 bytes) — BLOCKED (2026-10-10)

VU1 **explicit-UV sprite-quad** packet builder (8 int args: frame, x, y, w, h,
u, v, alpha) — the 8-param sibling of the already-blocked 2-corner
`func_00200078` and 4-corner `func_001FFE18`, and cousin of
`Hud_DrawSprite__Fiiiiii` (func_001FFC30). Callers: help.cpp (func_001FD748)
at 0x1FD814/0x1FD858, passing `(texFrame, 0, 0, w, h, u, 0, 0x80)`.
Source: `code/game/hud_post_post2_post2.cpp:11` (INCLUDE_ASM retained).

Blocked on EGC 2.95.2 scheduler / register-allocation tie-breaks. Semantics
are FULLY solved; best candidate (cand08) is **528/528 bytes with IDENTICAL
instruction multisets, 97 word-diffs, allocation 12/13 values correct** —
the sole register miss is `u` in a1 vs the original's t4, plus coupled
scheduling/interleaving choices. Every source/scheduler/pin lever regressed.
last-resort GPT-5.6 Sol used (its destructive-form recommendation is the best
candidate); expert GPT-6 Astra used (its `-fno-schedule-insns` lever regressed
to 520B/129).

## What it does

Appends a 16-byte header `[0x10000005, 0, 0, 0x50000005]` (four 32-bit stores
through fresh self-based lui/lw reloads of `vu1ChainHead`, a plain
non-.data double-volatile `u32*` at 0x160F00) then 10 qwords at head+0x10,
then advances head by 20 words (0x50):

- q0 = 0x7400000000008001 (64-bit const, materialized pre-call in s8/fp
  `ori $30,0xE800; dsll32 $30,15; ori $30,0x8001`, saved at 0xB0, stored at
  OLD head+0x10)
- q1 = 0x5353106 (t8, unsaved, stored at newhead+8 pre-call)
- q2 = GetFrameTex(frame) (the one jal at 0x200358; v0 return)
- q3 = 0x156 (in a1 in the original), q4 = (alpha<<24)|0x7F7F7F (s0)
- q5 = u|(v<<16); q7 = (u+texU)|((v+texV)<<16)
- q6 = (x+minX-8)|((y+minY-8)<<16)|(z<<32)
- q8 = (x+w+minX-8)|((y+h+minY-8)<<16)|(z<<32); q9 = 0

z = `hudHeap.vuField_0C` — a **signed int** (plain `lw` + `dsll32 0` = high
word); declaring it `u32` emits `lwu` (1-word diff) — the type is now `int`
in hud.h (shared with matched Hud_DrawChannels, which writes it via sw).
minX/minY = occlViewParams+0x10/+0x14, reloaded per corner (4 loads, must stay
uncached). texU = 1<<(uLog+4), texV = 1<<(vLog+4); uLog@tex+6, vLog@tex+7.

## Original register map

s0=alpha, s1=p=head+0x10, s2=w, s3=x, s4=h, s5=y, s6=v, s7=texV, s8/fp=q0,
t4={tex, then u REUSED after tex dies at 0x310}, t7=texU, t8=q1, t9=&hudHeap.
Frame 0xD0, 13 sq. u/texU sq's at 0(sp)/0x10(sp) immediately pre-jal; t9 sq in
the jal delay slot. u lq restore is FIRST post-call (0x360); lq ra at 0x3F0
(mid pos2 block); contiguous lq s8..s0 epilogue block 0x428-0x454 with lq s7
and the head lui/lw+addiu 0x50 interleaved; final GPREL store via at. a1 holds
the 0x156 const post-call.

## Winning source form (cand08, working/hud_post2_post2_func_00200258/)

- plain non-.data double-volatile `extern volatile u32* volatile vu1ChainHead;`
  (fresh self-based lui/lw pair per access, unhoisted — 8 pairs match)
- destructive in-place param updates: `h += y; w += x; v += texV;` then
  `y += minY; x += minX; y -= 8; x -= 8;` then `u += texU`
- seeded texU/texV: `int texU = 1; texV = texU << (tex->vLog+4);
  texU = texU << (tex->uLog+4);` (seed 1 in t7, clobbered to texU)
- early-staged `u64 packetTag = 0x7400000000008001ULL; u64 packetRegs = 0x5353106;`
- `volatile u64* q = (volatile u64*)head;` (preserves store order)
- `register int texV asm("$23") = ...` (s-register pin works)

This form (from last-resort GPT-5.6 Sol) produced the first exact-size
candidate (cand07: 528B/107, u<->texV swapped); cand08 added the texV->s7 pin
(528B/97, best).

## The 97 residual diffs (cand08 vs original)

- u in a1 (sq a1 0x358 / lq a1 0x3A4) vs t4 (sq t4 0x350 / lq t4 0x360 first);
  cascade: 0x7F7F7F in a0 vs v1, v-copy in v1 vs v0, 0x156 in v0 vs a1,
  occl base in a0 vs a2, minY in v0 vs v1, minX in v1 vs a0
- prologue: [x-copy, sq s7] vs [lui/ori t8] 4-word permutation; u-copy at
  0x2C4 (early) vs 0x310 (late, after tex dies); h+=y/w+=x HOISTED into the
  prologue (0x2E8/0x2EC) vs original 0x39C/0x3A0 post-call; lbu uLog/vLog
  order swapped; both +4 addius batched vs interleaved addiu/sllv; alpha copy
  early (0x30C) vs 0x314
- post-call: v+texV early (0x384 vs 0x3A4); lq ra early (0x398 vs 0x3F0);
  lq u late (0x3A4 vs 0x360); lq s7/s8/s0 scattered (restore-on-demand) vs
  contiguous epilogue block; u+texU early (0x3C4 vs 0x3D4); z load late
  (0x3E4 vs 0x3C4); q5 or/store at 0x3A8/0x3B0 vs 0x378/0x398

## Attempt matrix (all probed vs the generated asm)

| cand | form | result |
|------|------|--------|
| cand01 | exploratory | 132 diffs / 564B |
| cand02 | exploratory (broken semantics) | 127 / 512 |
| cand03 | natural, unpinned | 126 / 516 (full Reload divergence) |
| cand04 | pins incl. &hudHeap->t7 | 119 / 504 (t7 collided with texU) |
| cand05 | u->t4 pin at top, &hudHeap->t9 | 125 / 492 (u init copy DROPPED, uLog load DROPPED, t9 unsaved across jal) |
| cand06 | + q1->t8 pin | 125 / 492 (pin IGNORED) |
| cand07 | last-resort destructive form | **528 / 107** (first exact size; u<->texV swapped) |
| cand08 | cand07 + texV->s7 pin | **528 / 97** (best; 12/13 values) |
| cand09 | cand08 + LATE u->t4 pin (after tex) | 508 / 112 (cascading reshuffle: texs->t1 clobbered u param, copy dropped, texU->t6, &hudHeap->t7, q0->t9, s8 dropped, frame 0xB0) |
| probe10 | cand08 + `-fno-schedule-insns` (expert) | 520 / 129 (seed->t0, q0->t1, alpha->s1, u->a2; worse) |

## EGC 2.95.2 findings (verified on this target)

1. s-register (callee-saved) asm-pins are honored and safe (texV->s7, q0->s8,
   p->s1 all land correctly).
2. t-register (caller-saved) pins MISCOMPILE: u->t4 at function top dropped
   the init copy AND the whole uLog load (texU computation lost) when a
   hoisted const also wanted t4; q1->t8 pin ignored; &hudHeap->t9 live across
   a jal emitted NO save; a LATE u->t4 pin (tex already dead in t4) cascaded
   a full reshuffle (texs took t1 = the u param reg, clobbering u before its
   copy, which was then dropped).
3. This EGC restores callee-saved registers restore-on-demand (scattered lq's
   through the computation); the original binary has a contiguous epilogue
   block with lq ra placed mid-pos2 and lq s7 interleaved between the head
   lui/lw pair.
4. The original reuses t4 for u after tex dies (daddu $12,$9 at 0x310) and
   puts 0x156 in a1; local EGC's Reload gives u a1 and leaves t4=tex dead,
   pushing 0x156 to v0.
5. `-fno-schedule-insns` (first pass off) regressed the allocation entirely;
   the original was built with the default first scheduling pass.

## Escalations

- last-resort-decompiler (GPT-5.6 Sol), 2026-10-10, full dossier: verdict
  "not yet defensibly blocked"; recommended the destructive in-place
  param-update + seeded-texU + early-constant + volatile-u64*q form.
  Applied = cand07 (528B/107) -> cand08 (528B/97, best). Did not match.
- expert (GPT-6 Astra), 2026-10-10, full dossier (original asm + cand08 asm +
  attempt matrix): root-cause hypothesis = pre-RA scheduling changing
  live-range overlap for the u copy; recommended compiling cand08 unchanged
  with `-fno-schedule-insns` (second pass retained). Tested = probe10:
  520B/129, WORSE. Fallback blocker statement adopted (scheduling/allocation
  mismatch; caller-saved hard-register pin variants do not preserve the
  required computation).

## Retained state

- INCLUDE_ASM retained at code/game/hud_post_post2_post2.cpp:11.
- `hudHeap.vuField_0C` corrected to `int` (signed lw) in hud.h — shared with
  matched Hud_DrawChannels (sw writer, unaffected); full-ELF parity verified.
- Best candidate + all probe artifacts: working dir (cleared after block).
- Same coupled scheduler/RA wall class as func_001FFC30, func_00200078,
  func_001FFE18 — the whole HUD VU1 packet-builder family.
