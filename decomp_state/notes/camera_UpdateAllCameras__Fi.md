# UpdateAllCameras__Fi (0x1EC420, 0x110 = 272 bytes) — BLOCKED (66/68, prologue lui order)

Scans 48 UpdateCam slots (0xA0 stride) to pick a camera to activate.

## Outcome
Loop + tail + prologue ALL match except TWO swapped hi `lui` in the prologue
(0x1EC454/0x1EC458). 66 of 68 words match byte-for-byte. Not a byte-for-byte
match, so INCLUDE_ASM is retained.

## The 2 remaining words
```
        0x1EC454          0x1EC458
orig    lui $3, pUsed-hi  lui $2, pCam-hi
v14     lui $2, pCam-hi   lui $3, pUsed-hi
        (then) addiu $17, pCam-lo ; addiu $18, pUsed-lo   <- both match
```
Register assignment is identical (pCam->$2, pUsed->$3) and the LO order matches
(pCam-lo then pUsed-lo). Only the order of the two independent HI loads differs:
the original emits pUsed-hi ($3) before pCam-hi ($2); EGC emits them in
declaration order (pCam first). The original decouples the hi EMIT order from
the register/declaration order; EGC 2.95.2 does not (in any form I tried).

## What matches (the hard parts)
### Loop — key insight (expert GPT-6 Astra)
`beql`/branch-likely taken-delay-slot copies are mutually exclusive with the
fall-through. The original's three `pCam += 0xA0` are ONE source `pCam++` that
EGC duplicated into the taken-delay slots. Path sets: A(*pUsed==0)->I1,
B(pCam==cur)->I3, C(r==0)->I2, D(r!=0)->I3; each path advances pCam exactly once.
=> source is the SIMPLE chained `&&` with a SINGLE `pCam++`:
```c
do {
    if (*pUsed != 0 && pCam != cur &&
        (r = Camera_ActivationCheckPriority(pCam, cur)) != 0) {
        cur = pCam;
        changed = 1;
    }
    pCam++;
    i--;
    pUsed++;
} while (i >= 0);
```

### Tail — key insight (last-resort-decompiler GPT-5.6 Sol)
The tail matched once the CALL SIGNATURES were corrected (the earlier drafts
had wrong arg counts, which drove the whole tail's register allocation wrong):
- `func_001EBF10(cur)` is ONE-arg (the `lh` in the beql delay is a common
  funcIdx read, NOT an argument — the likely branch annuls it on the call path).
- `ExecuteCamPostUpdFuncs()` is ZERO-arg (no prepared a0 in the original).
- the collWithHero callback is ONE-arg `void(*)(UpdateCam*)` (the `lwc1 $f1`
  is NOT a callback arg; it feeds the lPos.x copy).
- `coll` is computed BEFORE `Camera_handleCollWithHero`, so it is live across
  that call -> EGC coalesces it into $s1($17): `lw $17,0xC($2); jalr $17`.
- the second arg of `Camera_handleCollWithHero((int)cur, (UpdateCam*)sizeof(UpdateCamVtbl))`
  is the SAME 0x14 constant the vtbl-index `mult` reuses -> one `li $5,20`.
- lPos copies as `float* pos=(float*)&cur->posQuad; float* dst=cur->lPos;
  dst[0..2]=pos[0..2];` -> the original's $f1/$f0 assignment and x,y,z store
  order with z in the ExecuteCamPostUpdFuncs delay slot.

## Winning candidate, 272 bytes, ndiff 2
Full C form preserved at decomp_state/notes/camera_UpdateAllCameras__Fi.candidate.cpp
(standalone: inline structs + externs + the function body below).
Prologue C (matches Deadlocked ref order pCam-then-pUsed and the original's
statement order cur/changed=0/Camera_Exit/i=0x2F/pCam/pUsed):
```c
UpdateCam* cur = (UpdateCam*)curCam;
Camera_Exit(cur);
int changed = 0;
int i = 0x2F;
UpdateCam* pCam = updateCams;
int* pUsed = updateCamsUsed;
```
Then the loop above, then the corrected tail.

## Tried for the 2 lui (none fixed the order with the correct reg assignment)
- v14 pCam declared first: pCam-hi first (the 2-diff baseline).
- v16 pUsed declared first: pUsed-hi first BUT wrong reg (pUsed->$2) -> 4 diffs.
- v15 declare pCam / init pUsed first then pCam: 4 diffs.
- v17 `int r;` declared before the pointers: same 2 diffs.
- v18 `register int* pUsed asm("$3")`: 63 diffs (pin clobbers the loop).
- --flags=-fno-schedule-insns: 33 diffs; -fno-schedule-insns2: 22 diffs.
The declaration order in EGC 2.95.2 sets BOTH the $2/$3 assignment and the hi
emit order in the same direction; the original needs them opposite.

## Types / data
- UpdateCam (inline-defined in candidate): posQuad@0x30 (floats x/y/z at
  0x30/34/38), lPos[3]@0x64, funcIdx(s16)@0x8C.
- UpdateCamVtbl: {int@0; fn@4; fn@8; collWithHero@0xC; fn@0x10} (0x14 stride).
- Symbols: curCam=0x1870c0, updateCams=0x187410, updateCamsUsed=0x189B50,
  lvlCamVtbl=0x1E8C00, func_001EBF10=0x1EBF10 (all absolute, out of gp window).
- `Camera_ActivationCheckPriority(int, UpdateCam*, UpdateCam*)` C-linkage;
  `Camera_Exit(UpdateCam*)`, `Camera_handleCollWithHero(int, UpdateCam*)` C++.

## Next / re-attempt
Only the 2 hi-lui order remains. A future EGC quirk note or a per-TU flag that
decouples hi-emit order from declaration order would close it. Otherwise this
stays blocked with the C form preserved at
decomp_state/notes/camera_UpdateAllCameras__Fi.candidate.cpp.
