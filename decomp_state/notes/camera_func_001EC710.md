# func_001EC710 (0x1EC710, 0xDC bytes) — BLOCKED (EGC scheduler tie-breaks; DImode folding defeated by barrier)

`code/game/camera.cpp:304`. No-arg void camera-transition helper, called twice
from the per-frame driver `Camera_TransitionFrame` (FUN_001ec8a0, 0x1EC8A0) on
the "pending transform" (reqType==2) paths.

## Semantics (Ghidra + objdump confirmed)

With `base` = level camera-data block at 0x13F350 and `blend` = camTransState
(`CamBlender`) at 0x1871B0 (both out of the gp window):

```c
vec4 fwd, side, up;                       // 16-byte stack locals
FastVecNormalize(&fwd,  (vec4*)(*(u32**)(base+0x2080) + 0xC0), 1.0f); // ptr reloaded per call
FastVecNormalize(&side, (vec4*)(*(u32**)(base+0x2080) + 0xD0), 1.0f);
FastVecNormalize(&up,   (vec4*)(*(u32**)(base+0x2080) + 0xE0), 1.0f);
*(CameraQuad*)(blend+0x90) = fwd;                       // 128-bit store
*(CameraQuad*)(blend+0xA0) = up;                        // 128-bit store
Camera_Pos2Polar3d((PolarSm*)(blend+0x70), (vec4*)(blend+0xC0),
                   (vec4*)(base+0x80), &fwd, &side, &up);
*(CameraQuad*)(blend+0xB0) = *(CameraQuad*)(blend+0xD0); // 128-bit copy
```

`*(u32**)(base+0x2080)` is reloaded (a fresh `lw a1,0x2080(s2)`) before every
call; base is held in `$s2`, blend in `$s0`, `&side` in `$s3`, `&up` in `$s1`.

## THE KEY FINDING: DImode folding IS defeatable (supersedes camera_func_001EC868 scope note)

The sibling note `camera_func_001EC868.md` documented that EGC 2.95.2 folds
128-bit (mode TI / `CameraQuad`, lowered to `lq`/`sq`) constant-base DATA
accesses into offset addressing (`sq v0,144(s0)`) and claimed no flag/source
form emits the original's materialized register-base form. **That conclusion is
wrong.** A tied READ/WRITE empty-asm barrier defeats it:

```cpp
CameraQuad* dst = (CameraQuad*)(blend + 0x90);
asm volatile("" : "+r"(dst));   // TIED read/write barrier — the "+r" is required
*dst = *(CameraQuad*)&fwd;      // emits: addiu v?,s0,0x90 ; lq v0,... ; sq v0,0(v?)
```

- The `"+r"` (read-write) constraint is REQUIRED. An input-only `"r"(dst)`
  barrier is insufficient — EGC retains the `base + const` equivalence and
  still folds. Verified on this target (probe2a: 30 diffs folded -> materialized).
- No compiler flag is needed; `-mno-split-addresses` does NOT affect the
  DImode store form (probe2 still folded).
- Register tie-breaks are controllable: pinning the 128-bit DATA value to a
  register (`register CameraQuad tmp asm("$2")` = v0) and the base pointer to a
  register (`register CameraQuad* dst asm("$4")` = a0) reproduces the
  original's data/base register choices (store data in v0, bases in v1/a0).

This unblocks the FOLDING for the whole camera DImode class. What remains
below is SCHEDULING, not folding.

## Original store region (objdump ground truth)

```
1ec788: addiu v1,s0,0x90 ; lq v0,0(sp)  ; sq v0,0(v1)   # blend+0x90 = fwd
1ec794: addiu a0,s0,0xA0 ; lq v0,0(s1) ; sq v0,0(a0)   # blend+0xA0 = up
1ec7a0: addiu a0,s0,0x70 ; addiu a1,s0,0xC0 ; addiu a2,s2,0x80
1ec7ac: move t0,s3 ; move t1,s1 ; jal Camera_Pos2Polar3d ; move a3,sp
1ec7bc: addiu v1,s0,0xD0 ; addiu s0,s0,0xB0 ; lq v0,0(v1) ; sq v0,0(s0)
1ec7cc: lq ra,0x70(sp) ; ... ; lwc1 f20,0x80(sp) ; jr ra ; addiu sp,sp,0x90
```

## The remaining blocker: 10 instruction-ORDER (scheduler) diffs

Best candidate (cand9) matches the prologue, all three FastVecNormalize calls,
store2 (base=a0, data=v0 from s1), and the copy's register choices (src=v1,
dst=mutated s0) byte-for-byte. The residual 10 diffs are PURE instruction
ordering (same instructions, different schedule):

1. store1: `addiu v1,s0,0x90` / `lq v0,0(sp)` order swapped (2 words).
2. call-arg setup is interleaved with store2's `sq v0,0(a0)` (6 words): EGC
   hoists the independent args (a1,a2,t0) before store2's sq and sets a0 last;
   the original keeps store2's sq contiguous and orders args a0,a1,a2,t0,t1.
3. final copy: `sq v0,0(s0)` / `lq ra,0x70(sp)` order swapped (2 words).

These are EGC scheduler tie-breaks. `-fno-schedule-insns` / `-fno-schedule-insns2`
make it WORSE (20/24/31 diffs) — the original used the default scheduler, so the
difference is a version/heuristic difference in the scheduler, not a flag.

## Tried (12 probes via tools/decomp_probe.py)

| form | result |
|------|--------|
| cand1: u8[] base, CameraQuad stores, no barrier | 42 diffs, folded stores, 0xCC |
| probe2: cand1 + -mno-split-addresses | still folded |
| cand2a: + tied "+r" barrier on each dst | 30 diffs, stores MATERIALIZED (folding defeated), 0xDC |
| cand2b: + pin sidePtr/upPtr | 39 (worse; hoisted) |
| cand2c: + pin base->s2 only | 21 diffs |
| cand3: cand2c + named pu | 21 |
| cand4: + pin dst90/dstA0/srcD0 | 25 (worse; prologue broke) |
| cand5: reused-blend dst, no barrier | 23, dst re-folded |
| cand6: + tmp90 local for store1 data | 12 |
| cand7: + pin dst90->v1, barrier pu | 13 |
| cand8: + pin tmp90->v0, barrier pu | 10 |
| cand9: + pin dstA0->a0 | **10 diffs (best)** |
| cand10: cand9 + base-first order | 10 |
| cand9 + -fno-schedule-insns / insns2 / both | 20 / 24 / 31 (worse) |

## Escalation

`last-resort-decompiler` (GPT-5.6 Sol) invoked 2026-09-23. It correctly
identified that the prior "EGC always folds, no flag disables it" conclusion
applied only while EGC could prove the pointer equals `base + constant`, and
recommended the tied read/write empty-asm barrier `asm volatile("" : "+r"(ptr))`
to destroy that equivalence. Tested: it defeats the DImode folding (the
documented blocker). The residual 10 diffs are scheduler tie-breaks the
recommendation did not (and could not) address. Recorded as blocked on the
scheduler; the folding finding is durable and recorded in AGENTS.md.
