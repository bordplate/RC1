# func_001FFE18 (0x1FFE18, 0x25C = 604 bytes) — BLOCKED (2026-10-10)

VU1 direct-data **sprite-quad** packet builder (4 corners). Same family as the
blocked 2-corner sibling `Hud_DrawSprite__Fiiiiii` (0x1FFC30,
`decomp_state/notes/hud_Hud_DrawSprite__Fiiiiii.md`). Blocked on EGC 2.95.2
default-scheduler / register-allocation tie-breaks: a robust **5-register
prologue cycle** plus body scheduler residuals. Best candidate = 604 bytes
(exact size) / 85 word-diffs (15 prologue + 70 body).

## What it does

`void f(int frame, int x, int y, int w, int h, int alpha)` (6 int params;
a0=frame, a1=x, a2=y, a3=w, t0=h, t1=alpha — EGC 8-arg ABI puts 5th/6th in
t0/t1).

```
texU = 1 << hudHeap.texs[hudHeap.frames[frame].hTex].uLog
texV = 1 << hudHeap.texs[hudHeap.frames[frame].hTex].vLog
```

Appends a 16-byte header `[0x10000007, 0, 0, 0x50000007]` (qcnt 7) plus 14
qwords at `qbase = vu1ChainHead + 16`, then advances the head by 112 bytes
(`vu1ChainHead += 0x1C` in u32 units):

- q0  = 0xB400000000008001
- q1  = 0x53535353106
- q2  = GetFrameTex(frame)            (jal 0x1FFA10, 1-arg, a0=frame)
- q3  = 0x154
- q4  = ((u64)alpha<<24) | 0x7F7F7F
- q5  = texU<<4
- q6  = cornerA = (z<<32)|(((y+h)<<4)+minY-8)<<16|((x<<4)+minX-8)
- q7  = (texV<<20)+(texU<<4)          (int `+`, not `|`, not u64)
- q8  = cornerB = (z<<32)|((y<<4)+minY-8)<<16|((x<<4)+minX-8)
- q9  = 0
- q10 = cornerC = (z<<32)|(((y+h)<<4)+minY-8)<<16|(((x+w)<<4)+minX-8)
- q11 = texV<<20
- q12 = cornerD = (z<<32)|((y<<4)+minY-8)<<16|(((x+w)<<4)+minX-8)
- q13 = 0

Corners (packed `(x,y,z)`): A=(x, y+h), B=(x, y), C=(x+w, y+h), D=(x+w, y).
`xN=(xN<<4)+minX-8`, `yN=(yN<<4)+minY-8`, `z=hudHeap.vuField_0C`.

## Key semantic correction (vs. a first wrong read)

The mid-body call at 0x1FFF08 is **`jal GetFrameTex__Fi` (0x1FFA10) with a
single arg (a0=frame)**; its return (`v0`) is stored into q[2]. It is NOT a
call to the 6-arg sibling `func_001FFC30`. A candidate that calls the sibling
with 6 args generates 5 extra arg-reload moves before the jal and never
matches. This was the real bug in the first candidates; fixing it (1-arg
`GetFrameTex(frame)`) was necessary but not sufficient.

## Verified data / register facts

- `occlViewParams` base = 0x13E500; `minX` at +0x10, `minY` at +0x14. Re-loaded
  once per corner (8 loads total, NOT CSE'd) → must be `volatile s32` in C.
- The base stays in **a2 ($6)** across all four corners (a1 is used as a temp
  for `x<<4`), so pin `register volatile struct OcclViewParams* vp asm("$6");`.
- `hudHeap.vuField_0C` read via `lw` (signed int), `dsll32` into the HIGH word
  (`(u64)z<<32`).
- `hudHeap`: frames +0x20, texs +0x24, vuField_0C +0x0C (0x19A3E8).
- Head form in this TU: plain (non-.data) double-volatile
  `extern volatile u32* volatile vu1ChainHead;` (self-based absolute loads).

### Original register map (verified)

s0=alpha, s1=x, s2=qbase, s3=h, s4=texU(base "1"), s5=y, s6=&hudHeap,
s7=texV(vLog), s8=w; a2=occlViewParams base; frame in a0; ra + s0–s8 saved
(10 `sq`) in an interleaved prologue order.

## The blocker: a 5-register prologue RA cycle

The param-copy ORDER already matches (y, dx, x, dy, alpha) and these allocate
correctly: dx→s8, alpha→s0, &hudHeap→s6, texV→s7. ONLY a 5-register cycle is
wrong (original → candidate):

```
texU  s4 → s3
y     s5 → s4
x     s1 → s5
qbase s2 → s1
h     s3 → s2
```

Plus the self-based head load (`lui r,0x16; lw r,0F00(r)`) is scheduled EARLY
in the original (right after the y copy, 0x1FFE88) but LATE in the candidate
(after the alpha copy, 0x1FFEA4). The 70 body diffs largely flow from this
cycle (registers carried through the body) plus scheduler tie-breaks — the
sibling had a MATCHING prologue and still carried 40 body diffs.

## Attempts (all on production default flags, `tools/decomp_probe.py`)

| cand | change                                    | size  | diffs | prologue |
|------|-------------------------------------------|-------|-------|----------|
| v5   | correct 1-arg GetFrameTex (baseline)      | 604   | 85    | 15       |
| v6   | v5 + body store order q4,q5,q2,q3         | 604   | 85    | 15 (no change) |
| v7   | v5 + tag stores before texU/texV          | 604   | 105   | 31 (worse) |
| v8   | v5 + hoist `volatile u32* firstHead` head read above heap reads | 604 | 105 | 35 (worse, shifts whole prologue) |
| v9   | v5 + `register int savedTexU asm("$20")=1<<uLog` + post-call tied transfer | 604 | 104 | 34 (worse, shifts sq interleaving) |

Every lever that tries to force one register of the cycle perturbs the
9-interleaved `sq` prologue save order and makes the match WORSE — exactly the
sibling's finding ("the prologue is extremely allocation-sensitive"; scalar /
param pins are IGNORED or harmful by this EGC).

## Escalations

- **last-resort-decompiler (GPT-5.6 Sol, 2026-10-10)**: two concrete probes.
  (1) Hoist only the first volatile head read ahead of the volatile `hudHeap`
  reads via a local `firstHead` (= v8 above) → 105 diffs / 35 prologue, worse
  (shifts the first `sll`). (2) If the 5-cycle remained, pin `texU` to s4 with
  a scoped post-call tied transfer `register int savedTexU asm("$20")=1<<uLog`
  + `asm volatile("" : "=r"(texU) : "0"(savedTexU))` after the call (= v9) →
  104 diffs / 34 prologue, worse (shifts the `sq` interleaving). Both tested
  separately; both worse than v5.

## Verdict

BLOCKED. Semantics fully solved (size matches, body store/corner logic correct,
call corrected to 1-arg GetFrameTex). The residual is a coupled EGC 2.95.2
default-scheduler / RA tie-break: a 5-register prologue cycle (texU,y,x,qbase,h)
plus body scheduling. Every scheduler/RA constraint tried perturbs the
allocation-sensitive prologue and regresses. Same blocker class as
`Hud_DrawSprite__Fiiiiii` (0x1FFC30). Retain INCLUDE_ASM; full-ELF parity
preserved (no source change committed).

## Re-attempt start point

`working/hud_func_001FFE18/cand_v5.cpp` (correct 1-arg GetFrameTex,
`vp asm("$6")`, plain double-volatile head, `(u64)z<<32`, int `+` texflags).
If revisited: make `OcclViewParams.minX`/`minY` `volatile s32` in camera.h and
`hudHeap.vuField_0C` a plain signed int in hud.h (both verified to be the
original's form, reverted here since the function stays INCLUDE_ASM). The
residual is the 5-register prologue cycle + body scheduler; any attempt needs
a constraint that does NOT touch the 9 interleaved s-register saves.
