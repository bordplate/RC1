# camera func_001ECCD8 (0x1ECCD8, 0x5D8 = 1496 bytes) — BLOCKED

Per-frame "polar" camera-blend transition step. Sibling of the matched
`Camera_TransitionStep` (0x1ECAF8): the caller `func_001ED2B0` dispatches
`if (camBlendType == 0) Camera_TransitionStep(pTarget, camTransState+0x10);
else func_001ECCD8(pTarget, camTransState+0x70);`. This is the mode!=0
(azimuth/elevation/radius polar blend) path.

## Params / frame
- a0 = `UpdateCam* pTgt`; a1 = `camTransState + 0x70` = `&CamBlender.polar`
  (a sub-view that extends past the 12-byte `PolarSm` into the following
  CamBlender fields). Prologue: s4 = a1, s5 = a0.
- Frame `addiu $sp,$sp,-0x1D0` (464); saves s0-s8 + ra (10 GP) and
  f20-f26 (7 FPR). One shared epilogue at 0x1ED264 (early `return 1` sets
  v0=1 and branches to it; the normal path sets v0=0 and falls in).

## Data / type layout established this session
- `a1` reads: `polar.azimuth/elevation/radius` (+0x00/04/08), `blendStep`
  (+0x0C, signed int, `FastDecTimer` decrements it), `blendStepInv` (+0x10),
  `reqInterpFrames` (+0x14), **`basis0` (+0x20)**, **`basis1` (+0x30)**,
  `blendWork` (+0x40), `pendingCam0` (+0x50), `pendingCam1` (+0x60).
  => CamBlender needs two NEW 16-byte fields, basis0 (0x90) and basis1
  (0xA0), currently inside `pad_88[0x28]`. (Shared-struct fix for when this
  is resumed — do NOT commit until the function matches.)
- Standalone symbol needed: `camBlendType` byte at 0x1871B2 (original loads
  `lui %hi; lbu %lo(0x1871B2)` => LO16 0x1B2; `camTransState.type` 0x1871B0+2
  would give LO16 0xB2 — wrong). `occlCamStaged`=0x18C32C. D_refMat=0x13F3D0,
  D_refBig=0x13F350 (pointer at +0x2080), D_refSide=0x13F5E0,
  currentCamera.pos=0x187080, orientMtx=0x187290.

## Body structure (from reference + Ghidra)
1. `if (pP->blendStep < 1) return 1;`
2. `factor = 1.0f / func_002133D0(1.0f, reqInterpFrames, blendStep*blendStepInv);` (factor held in $f24)
3. `if (camBlendType == 2)`: copy D_refMat->refPos(sp+0x40), pP->basis0->sp+0x10,
   pP->basis1->sp+0x30 (all direct lq/sq 128-bit); `FastVecCross(cross(sp+0x20), basis0, basis1)`;
   `Camera_Pos2Polar3d(&polar, &pTgt->posQuad, refPos, basis0, cross, basis1)` (6 args a0-a5, pos=pTgt+0x30).
   `else`: copy pTgt->posQuad->refPos(sp+0x40); `u32 rptr = D_refBig[0x820]`;
   `FastVecNormalize(basis0n(sp+0x10), rptr+0xC0, 1.0)`; `FastVecNormalize(basis1n(sp+0x30), rptr+0xE0, 1.0)`;
   polar = (azimuth=PI, elevation=0, radius=0).
4. `azDiff = FastSubRots(polar.az, pP->az);` (kept live in $f23 until FastAbsF at 0x1ECFF4);
   `pP->az = FastAddRots(pP->az, azDiff*factor);`
   `elDiff = FastSubRots(polar.el, pP->el); pP->el = FastAddRots(pP->el, elDiff*factor);`
   `pP->radius += (polar.radius - pP->radius)*factor;`
   `FastVecNormalize(camVec(sp+0x50), basis0n, pP->radius);`
5. `func_00214890(az, camVec, camVec, basis1n); FastVecCross(cross, camVec, basis1n);`
   `FastVecNormalize(cross, cross, 1.0); func_00214890(el, camVec, camVec, cross);`
   `FastVecAdd(pP->pendingCam0, refPos, camVec); if (!occlCamStaged) copy(pP->pendingCam0 -> currentCamera.pos);`
6. `func_001FA480(&pP->blendWork, basis@sp+0x80)` writes THREE 16-byte basis rows (sp+0x80/0x90/0xA0).
   `d1=FastVecDot(basis[2], pTgt); func_001F9A68(scaled, d1, basis[2]); FastVecSub(proj, pTgt, scaled);`
   `d2=FastVecDot(basis0n, proj); len=FastVecLength(proj); ang = pihalf - FastArcSin(d2/len);`
   `sideDot=FastVecDot(basis[1], proj); sign=(0<=sideDot)?1:-1; ang*=sign;`
7. **Angle-wrap block (0x1ECFF4-0x1ED094)** — the hard part: a chain of
   `c.lt.s/c.le.s` + `bc1f/bc1t` (branch-likely) FP ops over held FPRs
   f20=ang, f21=azStep/zero, f22=sign, f23=azDiff, f24=factor, f25=pihalf,
   f26=1.0, computing `azStep = ang*factor` with a wrapped-angle correction
   (`ang += 6.2831855f` / `ang -= 6.2831855f`) when `pihalf < FastAbsF(azDiff)`
   and the az/sign quadrant test holds. EGC schedules the shared
   `mul.s $f21,$f20,$f24` (azStep) into several mutually-exclusive
   taken-delay slots (4 copies) — see the 2026-09-23 AGENTS.md
   branch-likely duplication observation (one source increment, N delay copies).
8. Quaternion block: `if (FastAbsF(azStep)<1e-5){ q0=basis[1]; q1=basis[2]; }
   else { func_00214530(azStep, q2, basis[2]); func_00214800(q0, basis[0], q2); func_00214800(q1, basis[1], q2); }`
   `if (FastAbsF(ang)<1e-5){ q2=basis[1]; } else { func_00214530(ang, qTemp, basis[2]); func_00214800(q2, basis[0], qTemp); }`
   q0@sp+0xB0, q1@sp+0xC0, q2@sp+0xD0, qTemp@sp+0xE0.
9. `dot1=FastVecDot(q2, pTgt); asin2=FastArcSin(dot1); dot2=FastVecDot(q2, pTgt->mtx2); sgn=(0<=dot2)?1:-1;`
   `func_00214890((pihalf-asin2)*sgn*factor, q1, q1, q2);`
10. orientMtx rows: `FastVecNormalize(q[0], q1, 1.0); FastVecCross(q[1], q[0], D_refSide);`
    `FastVecNormalize(q[1], q[1], -1.0); FastVecCross(q[2], q[1], q[0]);`
11. `func_002144D8(pP->pendingCam1, orientMtx); func_002144D8(pP->blendWork, orientMtx);`
12. `FastDecTimer(pP->blendStep);` (takes `int&` => `FastDecTimer__FRi`); `return 0;`

## C-translation gotchas discovered (all applied in the working candidate)
- 128-bit (mode TI) compound lvalue assignment `*(CameraQuad*)a = *(CameraQuad*)b`
  and CameraMatrix by-value copy break EGC's C++ parser (cascade errors).
  Use the register-pinned copy form from the matched `Camera_TransitionStep`:
  `register CameraQuad* dst asm("$3"); register CameraQuad* src asm("$4");
  asm volatile("" : "+r"(dst),"+r"(src)); register CameraQuad v asm("$2") = *src; *dst = v;`
- `asm("...")` must be on a forward declaration, not the definition.
- Linkage: C++ mangled = FastArcSin__Ff, FastSubRots__Fff, FastAddRots__Fff,
  FastDecTimer__FRi (`int&`); C/unmangled = FastVec*, FastAbsF, FastCos,
  func_002133D0, func_002144D8, func_001F9A68, func_001FA480, func_00214800,
  func_00214530, func_00214890, Camera_Pos2Polar3d__FP7PolarSmP4vec4N41.
- `struct vec4 {f32 x,y,z,w;}` (mobyutil.h) needed for the Camera_Pos2Polar3d signature.

## last-resort-decompiler escalation (invoked this session, per workflow)
Returned a concrete, non-wall assessment: the candidate was structurally/
semantically wrong (single `Vec4` where func_001FA480 writes three 16-byte
basis rows; mode-2 copies from pP not pTgt; FastMemCopy where the original
has lq/sq; missing azDiff liveness). Recommended: 0xF0-byte aggregate
(PolarSm, basis0n, cross, basis1n, refPos, camVec, proj, scaled, basis[3],
q0,q1,q2,qTemp), register-pinned 128-bit copies, azDiff kept live, the
nested-conditional angle-wrap shape, and **default flags only** (no
-fno-schedule-insns / -mno-split-addresses); checkpoint `.frame $sp,464`,
`vars=240`, 7 FPRs f20-f26. Implemented exactly.

## Current state after implementing the recommendation
Second candidate compiles (default flags, SN): frame **480** (orig 464),
`vars=256` (orig 240), 10 GP + 7 FPR saves (matches). 382/374 words differ;
a few matching islands (+0x04C-0x054, +0x354-0x358, +0x3B0-0x3C8, +0x618-end).
The 16-byte frame overage (one extra spilled local — likely `rptr`) shifts
every `addiu $x,$sp,N`, and the f20-f26 FP register/scheduling of the
angle-wrap + quaternion blocks differs. So the structure is right but the
exact local layout and FP schedule are not reproduced.

## Concrete next steps to resume (in order)
1. Close the 16-byte frame gap: the candidate spills one extra 16-byte local
   (vars=256 vs 240). Inspect `probe2/candidate.s` sp-offsets vs the reference
   to find the extra slot; likely keep `rptr` (D_refBig[0x820]) register-resident
   or restructure the two FastVecNormalize calls so it isn't spilled, aiming
   for `.frame $sp,464` / `vars=240`.
2. With the frame right, re-diff; the sp-offset diffs should collapse to the
   genuine FP-scheduling deltas in the angle-wrap block (0x1ECFF4-0x1ED094)
   and the quaternion block.
3. The angle-wrap block: the 4 duplicated `mul.s $f21,$f20,$f24` are EGC
   taken-delay-slot copies of ONE `azStep = ang*factor` (do not emit 4).
   The nested-conditional shape in the working candidate should produce the
   right CFG; if FPR names differ, pin f20=ang, f21=azStep, f22=sign,
   f23=azDiff, f24=factor, f25=pihalf, f26=1.0 as scoped hard-regs.
4. 128-bit copy sites each use DIFFERENT register pins in the original
   (not all $3/$4/$2); the macro pins all to $3/$4/$2 and will need per-site
   pins to match exactly.
5. Commit only after the probe matches AND the full `make` + `cmp` passes, and
   include the CamBlender basis0/basis1 (0x90/0xA0) shared-struct fix + the
   `camBlendType` symbols.txt entry.

## Probe command
```
source .venv/bin/activate
python3 tools/decomp_probe.py working/camera_func_001ECCD8/candidate.cpp \
  code/_generated/nonmatchings/game/camera/func_001ECCD8.s func_001ECCD8 \
  --out working/camera_func_001ECCD8/probeN \
  --define camBlendType=0x1871B2 --define D_refBig=0x13F350 \
  --define D_refMat=0x13F3D0 --define D_refSide=0x13F5E0 \
  --define func_001FA480=0x1FA480 --define func_001F9A68=0x1F9A68 \
  --define func_00214800=0x214800 --define func_00214530=0x214530 \
  --define func_00214890=0x214890
```
(working/camera_func_001ECCD8/ was cleared after the blocker was recorded;
reconstruct the candidate from this note — the body map, the C-translation
gotchas, and the last-resort aggregate layout are all captured here.)
