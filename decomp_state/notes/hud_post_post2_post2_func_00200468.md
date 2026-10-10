# func_00200468 (0x200468, 0x194 = 404 bytes) — BLOCKED (2026-10-10)

VU1 data-ref **sprite** packet builder (scroll/blend/alpha variant of the
already-blocked HUD VU1 family). Blocked on EGC 2.95.2 default-scheduler /
caller-saved register-allocation tie-breaks in the prologue hole and the
q-word setup block. Best verified candidate (v20, production default flags) =
**404/404 bytes (exact size), 55 word-diffs**; the s-register allocation
(s0=q5, s1=&hudHeap, s2=q1, s3=q0, s4=q3), the q-store order (with the v24
reorder), the mid-function head write, and the tail store-before-lq are all
byte-correct.

Source: `code/game/hud_post_post2_post2.cpp:13` (INCLUDE_ASM retained).

## What it does

`void func_00200468(u64 tex, int x, int y, int logW, int logH, int dx, int dy,
int scroll, int blend, int alpha)` — 10 args: a0-a3, t0-t3, then sp+0x50
(blend), sp+0x58 (alpha). All six callers are in pause_post2 FUN_00220850
(0x2208f8/95c/9b4/a60/ab4/af0).

Appends a 16-byte header `[0x10000005, 0, 0, 0x50000005]` plus 10 qwords at
head+0x10 to the `vu1ChainHead` VU1 chain, **with a mid-function write**
`vu1ChainHead = p` (p = (u32*)vu1ChainHead + 4 words) issued BEFORE the q
stores (original 0x20053C-0x200540, `sw a3, %lo(vu1ChainHead)(at)`), then a
final `vu1ChainHead = vu1ChainHead + 20` words (byte +0x50). Both writes are
present in the bytes (verified against the Lombyte reconstruction
`reference/Lombyte/src/assembly/textbin/fun_00200468.c`); a single-write
candidate is 8 bytes short (v1, 396/85).

- q0 = 0x7400000000008001 (materialized `li 0xE800; dsll32 0xF; ori 0x8001`)
- q1 = 0x5353106; q2 = tex (a0); q3 = 0x156
- q4 = (alpha<<24) | 0x7F7F7F
- q5 = scroll | (blend<<16)
- q6 = (x+minX-8) | ((y+minY-8)<<16) | ((u64)vuField_0C<<32)
- q7 = (scroll + (1<<(logW+4))) | ((blend + (1<<(logH+4)))<<16)
- q8 = (x+dx+minX-8) | ((y+dy+minY-8)<<16) | ((u64)vuField_0C<<32)
- q9 = 0

minX/minY = occlViewParams+0x10/+0x14 (0x13E500); vuField_0C =
hudHeap+0x0C (0x19A3E8, signed int, plain `lw` + `dsll32`). All six
`vu1ChainHead` loads are fresh self-based `lui r,0x16; lw r,0F00(r)` pairs —
required form: plain (non-.data) double-volatile
`extern volatile u32* volatile vu1ChainHead;`. Store order q0,q1,q3,q5,q2,q4
(original); q0 stored via base head+0x10, the rest via base p=head+0x10.

## Winning source form (v20, 404/404, 55 diffs)

```cpp
volatile u32* head = vu1ChainHead;
u64 q4 = ((u64)alpha << 24) | 0x7F7F7F;
u64 q5 = scroll | ((u64)blend << 16);
head[0] = 0x10000005;
vu1ChainHead[1] = 0;
vu1ChainHead[2] = 0;
vu1ChainHead[3] = 0x50000005;
u32* p = (u32*)vu1ChainHead + 4;
vu1ChainHead = p;                 // mid-function write
u64* q = (u64*)p;
q[0] = ((u64)0xE800 << 47) | 0x8001;
q[1] = 0x5353106; q[2] = tex; q[3] = 0x156; q[4] = q4; q[5] = q5;
q[6] = (x + occlViewParams.minX - 8)
     | ((u64)(y + occlViewParams.minY - 8) << 16)
     | ((u64)hudHeap.vuField_0C << 32);
q[7] = scroll + (1 << (logW + 4))
     | ((u64)(blend + (1 << (logH + 4))) << 16);
q[8] = (x + dx + occlViewParams.minX - 8)
     | ((u64)(y + dy + occlViewParams.minY - 8) << 16)
     | ((u64)hudHeap.vuField_0C << 32);
q[9] = 0;
asm volatile("" : : : "memory");   // keeps the final load/add late
vu1ChainHead = vu1ChainHead + 20;
asm volatile("" : : : "memory");   // keeps the final store before the lq's
```

The two trailing `memory` barriers are load-bearing: without the first the
final load+add hoists into the first corner block (81 diffs); without the
second the lq restores run before the final store (63 diffs). With both: 55.
(v24 = v20 with q[0..5] store statements reordered q0,q1,q3,q5,q2,q4 also
scores 55 and matches the original store order exactly; use it if revisiting.)

## The 55 residual diffs (all scheduler / caller-saved RA tie-breaks)

1. **Prologue RA**: first head load t4(orig) vs v1(cand); tag2 0x50000005
   t7(orig) vs t4(cand).
2. **Prologue hole fill (0x2004A0-0x2004D8)**: orig fills the post-s-save hole
   with the arg fixups + stack-arg loads (`addiu a3,a3,4`; `lw v1,88(sp)`
   alpha) then the q0 const; cand hoists the independent consts (q0
   li/dsll32/ori, q1 lui/ori, 0x7F7F7F lui/ori, blend zext) into the hole and
   delays the alpha load to 0x2004D0 and `addiu a3` to 0x2004F8.
3. **q4 halves**: orig keeps alpha<<24 in v1 and 0x7F7F7F in v0, OR into v1;
   cand: 0x7F7F7F in v1, alpha<<24 in t7, OR into t7.
4. **head[2] store**: orig reloads head into t4 (0x2004F0) and stores at
   0x200504; cand reuses the v0 from 0x2004E8 and stores at 0x2004FC — no
   reload — cascading 0x2004F0-0x200524.
5. **q base p=head+0x10**: a3(orig) vs t0(cand) — write#1 store value plus the
   nine sd bases (~14 words, 0x200534-0x2005C8).
6. **Corner x/y micro-order (0x20057C-0x200588, 0x2005B0-0x2005B4)**: orig
   keeps y ahead (`addiu a2; dsll a2; addiu a1; or a1`); cand batches the x
   ops (`addiu a1; dsll a2; dsll32 v0; or a1`).

## Attempt matrix (all probed vs the reference .s, production default flags)

| cand | form | result |
|------|------|--------|
| v1 | first 10-arg form, single head write | 396/85 (8 B short — missed mid-write) |
| v2 | + mid-write, pointer-arith bug (+0x140) | 404/92 |
| v3 | fix advance to +0x50 bytes | 404/92 |
| v4 | correct double-write structure | 404/86 |
| v5 | q4/q5 named locals before q stores | 404/81 |
| v5f1 | v5 + -fno-schedule-insns | 372/100 |
| v5f2 | v5 + -fno-schedule-insns2 | 404/89 |
| v6 | v5 + head->t1 pin + tag2->t3 pin + tag1 local + barrier (WRONG reg nums) | 412/96 |
| v6a | v6 minus head pin | 408/90 |
| v6b | v5 + pre-final `memory` barrier | 404/63 |
| v7 | v6b, q4/q5 declared before header stores | 400/87 |
| v8 | v6b + tag2 pin (wrong reg $11) | 404/63 (ignored) |
| v9/v10 | v6b + q-base->a3=$7 pin | 416/98 (cascades) |
| v11 | v6b + store stmt order q0,q1,q3,q5,q2,q4 | 404/63 (order matches, q regs shift) |
| v12 | v6b + q5/q4 local swap | 404/63 |
| v14 | v11 + swap | 404/63 |
| v15 | v6b + scoped head seed->t4=$12 + zero-byte transfer | 404/75 (tag2->s0) |
| v16 | v6b + tag2 pin CORRECT t7=$15 | 404/63 (pin IGNORED) |
| v17 | v15 + v16 | 404/75 |
| v18 | v6b + tied `"+r"(q)` barrier | 388/99 |
| v19 | v6b + -mno-split-addresses | 404/72 |
| v20 | v6b + POST-store `memory` barrier (last-resort lever 1) | **404/55 (best)** |
| v21 | v6b + split alphaBits + destructive logW/logH += 4 (last-resort lever 2) | 404/63 (no effect) |
| v22 | v20 + v21 | 404/55 |
| v23 | v20 + alphaBits split + input barrier pin | 388/100 (shatters) |
| v24 | v20 + store stmt reorder | 404/55 (store order byte-matches) |
| v25 | v20 + q5/q4 swap | 404/55 |
| v26 | v20 + bare alpha split (no barrier) | 404/55 |

## Escalations

- **expert (GPT-6 Astra, one-shot, 2026-10-10)**: could not read the dossier
  file (tool-restricted) but prescribed 3 experiments: (1) tied `"+r"(q)`
  barrier -> v18 = 388/100 (worse); (2) scoped head seed->t4=$12 with
  zero-byte transfer -> v15 = 404/75 (perturbs: tag2 kicked to s0); (3)
  -mno-split-addresses -> v19 = 404/72 (worse). None beat v6b.
- **last-resort-decompiler (GPT-5.6 Sol, 2026-10-10)**: prescribed (1) a
  SECOND `memory` barrier immediately after the final store -> v20 = 404/55
  (WORKED: final store now before the lq restores, 8 diffs closed); (2) split
  `alphaBits = (u64)alpha<<24` + destructive `logH += 4; logW += 4;` -> v21 =
  404/63 (no effect; v22 combined = 55). Concluded that if the follow-ups
  regressed, blocking is justified. Follow-ups (v23-v26, both flags
  re-confirmed worse) did not beat 55.

## Verdict

BLOCKED. Exact size (404/404), identical callee-saved allocation
(s0-s4 = q5/hud/q1/q0/q3), correct store order, correct tail, and both
volatile-write semantics are byte-correct; the 55 residuals are coupled
EGC 2.95.2 default-scheduler / caller-saved Reload tie-breaks (prologue
hole fill, head/t4 vs v1, tag2/t7 vs t4, q4 half registers, q base a3 vs t0,
corner micro-order). Every lever — both scheduler flags, -mno-split-addresses,
correct-register t/s pins (ignored or cascading), scoped seed transfers,
tied/input/memory barriers in five placements, destructive param updates,
alpha split, all source/store reorderings — was probed and failed to reduce
55. Same irreducible wall class as the blocked siblings in this file:
func_001FFC30 (Hud_DrawSprite), func_001FFE18, func_00200078, func_00200258
(Hud_DrawSpriteUV). Retain INCLUDE_ASM; full-ELF parity preserved (no source
change committed).

## Re-attempt start point

v20/v24 source (above). If revisited: the residual is (a) which independent
instructions fill the post-s-save hole and (b) caller-saved re-use of just-died
registers (a3 vs t0 for the q base). Any new lever must not touch the
s0-s4 allocation or the two trailing memory barriers. A future EGC/SN
version difference is the most plausible way this class resolves.
