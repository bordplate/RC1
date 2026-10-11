# func_00200958 (0x200958, 0x1B8 = 440 bytes) — BLOCKED (2026-10-11)

VU1 data-ref **textured-quad** packet builder (GetFrameTex variant of the
already-blocked HUD VU1 family). Blocked on EGC 2.95.2 default-scheduler /
register-allocation tie-breaks: a single s1<->s2 (p/alpha) swap plus a
3-instruction or/q9 interleaving in corner 2. Best verified candidate (v17,
production default flags, SN assembler) = **440/440 bytes (exact size),
17 word-diffs** — the best result in this file's family (siblings blocked at
48-97).

Source: `code/game/hud_post_post2_post2.cpp:17` (INCLUDE_ASM retained).
NOTE: the function needs an SN-assembled TU to match (bare
`lw r,vu1ChainHead` / `sw r,vu1ChainHead` pseudos expand self-based absolute
under ps2eeas; under the production GNU TU they expand GPREL16 — 109 diffs /
408 B). A Splat boundary split (GetFrameTex precedent,
hud_post_post2_gftex.cpp) would be required before any match can be committed.

## What it does

`void Hud_DrawFrameSprite(int frame, int x1, int y1, int x2, int y2,
int u0, int v0, int u1, int v1, int alpha)` — 10 args: a0-a5, t2, t3, then
sp+0xB0 (v1), sp+0xB8 (alpha). Both callers are in DrawSoundMenu (0x21CE00)
at 0x21CFA0 and 0x21D0A8.

Appends a 16-byte header `[0x10000005, 0, 0, 0x50000005]` plus 10 qwords at
head+0x10 to the `vu1ChainHead` VU1 chain (0x160F00), **with a mid-function
write** `vu1ChainHead = p` (p = head+4 words, absolute `lui at; sw` — NOT
GPREL) issued BEFORE the q stores, a mid-body `GetFrameTex(frame)` call
(0x1FFA10, u64 return in v0, 1-arg) feeding q2, then a final
`vu1ChainHead = vu1ChainHead + 20` words (+0x50 bytes), also absolute.
Store order: q0 (via the OLD head base), q1 (in the jal delay slot), q4, q5,
q2, q3, q7, q6, [lq ra mid-body], q9 (zero FIRST), q8; the final head
load is interleaved with the lq restores (lq s8, [lui/lw], lq s7, [addiu
v0,0x50], lq s6..s0, `lui at; sw`).

- q0 = 0x7400000000008001 (materialized `li t5,0xE800; dsll32 t5,0xF;
  ori t5,0x8001` — the SAME constant as func_00200468, which local EGC
  materializes in matching form; NOT the dli wall of func_00200600)
- q1 = 0x5353106; q2 = GetFrameTex(frame); q3 = 0x156
- q4 = (alpha<<24) | 0x7F7F7F
- q5 = u0 | (v0<<16)
- q6 = (x1+minX-8) | ((y1+minY-8)<<16) | ((u64)vuField_0C<<32)
- q7 = u1 | (v1<<16)
- q8 = same with x2/y2
- q9 = 0

minX/minY = occlViewParams (0x13E500) +0x10/+0x14, base materialized ONCE
(lui a1, 0x14 / addiu a1,-6912) and FIELDS reloaded per corner, minY load
FIRST in each corner. vuField_0C = hudHeap (0x19A3E8) +0x0C, base in a2
(materialized once, lui a2,0x1a / addiu -23576), field reloaded per corner
as plain `lw` + `dsll32 r,r,0` zero-extend. u1 (t3) gets NO callee-saved
home: `sw t3,0(sp)` in the prologue, reloaded `lw v0,0(sp)` mid-body for q7.
All six head loads are fresh self-based `lui r,0x16; lw r,0xF00(r)` pairs —
required form: plain (non-.data) double-volatile
`extern volatile u32* volatile vu1ChainHead;`.

## Winning source form (v17, 440/440, 17 diffs)

```cpp
#include "common.h"
#include "types.h"
#include "hud.h"
#include "camera.h"

extern volatile u32* volatile vu1ChainHead;

u64 GetFrameTex(int frame);

void Hud_DrawFrameSprite(int frame, int x1, int y1, int x2, int y2,
                         int u0, int v0, int u1, int v1, int alpha) {
    volatile u32* head = vu1ChainHead;
    head[0] = 0x10000005;
    vu1ChainHead[1] = 0;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = 0x50000005;
    u32* p = (u32*)vu1ChainHead + 4;
    vu1ChainHead = p;                 // mid-function write
    u64* q = (u64*)p;
    q[0] = ((u64)0xE800 << 47) | 0x8001;
    q[1] = 0x5353106;
    u64 tex0 = GetFrameTex(frame);
    q[4] = ((u64)alpha << 24) | 0x7F7F7F;
    q[5] = (u64)u0 | ((u64)v0 << 16);
    q[2] = tex0;
    q[3] = 0x156;
    volatile struct OcclViewParams* ovp =
        (volatile struct OcclViewParams*)&occlViewParams;
    volatile HudHeap* heap = (volatile HudHeap*)&hudHeap;
    int minY1 = ovp->minY;
    int minX1 = ovp->minX;
    q[6] = (u64)(x1 + minX1 - 8)
         | ((u64)(y1 + minY1 - 8) << 16)
         | ((u64)heap->vuField_0C << 32);
    q[7] = (u64)u1 | ((u64)v1 << 16);
    int minY2 = ovp->minY;
    int minX2 = ovp->minX;
    u64 corner2 = (u64)(x2 + minX2 - 8)
                | ((u64)(y2 + minY2 - 8) << 16)
                | ((u64)heap->vuField_0C << 32);
    volatile u64* tail = (volatile u64*)q;
    tail[9] = 0;
    tail[8] = corner2;
    vu1ChainHead = vu1ChainHead + 20;
}
```

Three structural discoveries made v17 work (each a measured step):

1. **No trailing memory barriers.** This original has the final head STORE
   AFTER the lq restores (the opposite of func_00200468, whose winning form
   uses two `memory` barriers to force store-before-lq). With the sibling
   barriers: 73 diffs; without: 52 (v2).
2. **minY/minX as explicit field locals, Y-first, with an x-first OR tree**
   (expert GPT-6 Astra recommendation): `int minY1 = ovp->minY; int minX1 =
   ovp->minX;` before the q[6] expression. This decouples the load order
   (minY first, matching the original exactly) from the OR accumulation base
   (x-part long-lived, matching the original). y-first expressions (v4) fix
   the load order but flip the OR bases and the q8/q9 order (48); x-first
   inline volatile loads (v2) fix the bases but flip the load order (52).
   The separated locals get both right: 33 (v9).
3. **Volatile tail stores through a separate pointer, q9 BEFORE q8, with
   corner2 precomputed** (last-resort GPT-5.6 Sol recommendation):
   `u64 corner2 = ...; volatile u64* tail = (volatile u64*)q; tail[9] = 0;
   tail[8] = corner2;`. The plain `q[9] = 0; q[8] = ...;` form lets EGC
   hoist the final head lui/lw into the corner-2 gap and store q8 first (33);
   the volatile tail form pins the final head load after the q stores and
   restores q9-before-q8: **17** (v17). This also flipped the ovp base
   register to a1 and the whole corner load-destination cascade to the
   original's (v1/a0/v0) — 16 of the 33 diffs closed at once.

## The 17 residual diffs (all scheduler / RA tie-breaks)

Class A — a single s1<->s2 swap, p/alpha (14 words): the original assigns
`s1 = p` (addiu s1,v1,16 at 0x200a04) and `s2 = alpha` (lw s2,184(sp) at
0x2009c8); v17 assigns `s2 = p` and `s1 = alpha`. The reservation SLOTS are
identical (alpha's sq is 2nd in the prologue at 0x96c in both, p's is 9th at
0x9b0 in both, the alpha load sits at 0x9c8 in both) — only the register
chosen for each slot differs. Cascades: the mid-write `sw`, the jal-delay
`sd t6,8(.)`, `dsll (alpha<<24)`, `or (q4)`, and the nine q-store bases
(16/24/32/40/48/56/64/72 + q1). Everything else matches: s0=v0, s3=x1,
s4=y1, s5=x2, s6=y2, s7=q7Hi(v1), s8=u0 (the original's consecutive
parameter run a1->s3..a4->s6, stack1->s7), the prologue sq order, the
constant hole, all 6 head loads, both head stores via $at, the t3
spill/reload, both corners' full instruction sequences and interleaving, the
epilogue lq/addiu shape.

Class B — corner-2 or/q9 interleaving (3 words, 0x200ac4-0x200acc):
original `or s5,s5,v0; sd zero,72(s1); sd s5,64(s1)`; v17
`sd zero,72(s2); or s5,s5,v0; sd s5,64(s2)`. The independent q9 (zero) store
is scheduled ahead of corner2's final OR in v17.

## Attempt matrix (all probed vs the reference .s, SN assembler, production flags)

| cand | form | result |
|------|------|--------|
| v1 | x-first, TWO trailing `memory` barriers (00200468-winning form) | 440/73 (epilogue wrong: final store must be AFTER lq's here) |
| v2 | x-first, no barriers | 440/52 (two clean s-swaps: s1/s2 AND s6/s7; minX/minY load order flipped) |
| v3 | v2 + 4 register pins (p->s1, alpha->s2, y2->s6, q7Hi->s7 as register locals) | 452 B (frame grew 0xB0->0xB8), 88 |
| v4 | y-first corner expressions, no barriers | 440/48 (load order fixed; OR bases + q8/q9 order flipped; s-scatter) |
| v5 | v2 + -fno-schedule-insns | 440/105 |
| v6 | v2 + -fno-schedule-insns2 | 440/100 |
| v7 | v2 + pin p->s1 only | 452 B (frame grew), 88 |
| v8 | v2 + pin alpha->s2 only | 440/52 (pin IGNORED, byte-identical mapping to v2) |
| v9 | EXPERT: minY/minX field locals (Y-first load) + x-first OR tree | **440/33** (ovp base + cascade fixed; p/alpha swap + or/q9 remain) |
| v10 | v9 right-associated OR tree | 440/57 |
| v11 | v9 + pin p->s1 | 452 B, 90 |
| v12 | v9 + pin alpha->s2 | 440/33 (pin IGNORED) |
| v13 | v9 with p born before the header stores | 440/76 (head load order broken) |
| v14 | v9 + single pre-advance `memory` barrier | 440/51 (perturbs the prologue sq/copy order) |
| v15 | LAST-RESORT: direct volatile symbol access for occlViewParams (asm-aliased extern) | 440/33 (no change) |
| v16 | LAST-RESORT: s64 v0 param + asm name override | cfront parse error (rejected per its own criterion) |
| v17 | LAST-RESORT: precomputed corner2 + `volatile u64* tail` stores, q9 then q8 | **440/17 (best, deterministic on re-probe)** |
| v18 | v17 + input-only `asm volatile("" : : "r"(corner2))` before tail[9] | 440/42 (barrier perturbs the prologue backward) |
| v19 | v17 + named-register PARAMETER (register int alpha asm("$18")) | cfront rejects named-register parameters |
| v20 | v17 + q4/q5 precomputed before the header stores (00200468-winning shape) | 432 B (8 B short), 98 |

Pin mechanics observed on this target: `register int x asm("$NN") = param;`
is SILENTLY IGNORED (v8/v12: the value still lands in the unpinned register,
byte-identical results); `register u32* p asm("$NN") = ...` (pointer local)
GROWS THE FRAME by 8 B and cascades (v3/v7/v11) — the pin apparently breaks
the q/p register coalescing. Named-register parameters are a cfront error.

## Escalations

- **expert (GPT-6 Astra, one-shot, 2026-10-11)**: verdict "currently
  unmatched, likely coupled RA/scheduler blocker, not proven irreducible";
  prescribed two experiments: (1) separate the Y-first volatile load order
  from the X-first OR tree via explicit minY/minX field locals -> v9 = 33
  (WORKED, the biggest single drop); (2) right-associate the OR tree -> v10
  = 57 (failed).
- **last-resort-decompiler (GPT-5.6 Sol, 2026-10-11)**: prescribed three
  variants: (1) direct volatile symbol access for occlViewParams -> v15 = 33
  (no effect); (2) s64 seventh param -> v16 cfront parse error (rejected per
  its own "reject immediately" criterion); (3) precomputed corner2 +
  volatile tail stores in original q9-before-q8 order -> v17 = 17 (WORKED:
  closed the epilogue hoist, the q8/q9 order, the ovp base register, and the
  entire corner load-destination cascade — 16 of 33 diffs). Concluded that if
  the follow-ups fail, "the exact-size 33-word residual is an irreducible
  coupled EGC 2.95.2 Reload/default-scheduler tie-break". Follow-ups (v18-v20)
  all regressed or failed.

## Verdict

BLOCKED. Exact size (440/440), correct everything except one 2-register
swap (s1/s2 = p/alpha, 14 words) and one 3-instruction interleaving (or/q9
in corner 2). Every lever probed failed: both scheduler flags, all pin
styles (ignored or frame-growing or cfront-rejected), memory barriers in
three placements (each perturbs the prologue), tied/input barriers, source
reorderings (y/x-first, p birth order, q4/q5 precompute), right-association,
direct-symbol vs pointer-field access, s64 param type. Same irreducible wall
class as the blocked siblings in this file: func_001FFC30 (Hud_DrawSprite),
func_001FFE18, func_00200078, func_00200258 (Hud_DrawSpriteUV),
func_00200468, func_00200600 — with the best residual of the family (17).
Retain INCLUDE_ASM; no source change committed; full-ELF parity preserved.

## Re-attempt start point

v17 source (above), probe with `--assembler=snas` (an SN TU split would be
required first if a match is ever found: config/RC1.yaml boundary at 0x200958,
new SN file, tail moved to a new GNU file, Makefile GNU list, symbols.txt).
If revisited: the residual is (a) which s register EGC gives the 2nd and 9th
prologue reservation slots (alpha vs p) and (b) whether the independent q9
zero-store precedes corner2's final OR. Any new lever must keep the
minY/minX-local + volatile-tail structure (removing either regresses 16+
diffs). A future EGC/SN version difference is the most plausible way this
class resolves.
