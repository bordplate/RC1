# Camera_TransitionFrame — func_001EC8A0 (0x254 = 596 bytes)

`code/game/camera.cpp`, `code/_generated/nonmatchings/game/camera/func_001EC8A0.s`.
Per-frame driver for the camera blend state machine (`camTransState`, 0x1871B0,
`CamBlender`). Dispatches on `state` (0 / 1 / else) with a `reqType`
sub-dispatch, then a common tail that latches `state = 3` and either copies the
direct (mode 0) pose cluster or advances the polar blend.

Blocked 2026-09-23 after ~10 candidate variants (working/camera_func_001EC8A0/
cand1–cand10). Full-ELF parity preserved by retaining the INCLUDE_ASM.

## Semantics (verified against Ghidra + objdump of assets/boot_elf.elf)

- Prologue: frame 0xB0 (0x28 callee words: s0–s5, ra, f20); `lui v0,0x18;
  lh v1,0x71b0(v0); addiu s4,v0,0x71b0` (s4 = base, invariant for the whole
  body); `li a0,1; bne v1,a0,` mode==1 block.
- Dispatch on state, each path a sub-dispatch on `reqType` (loaded once per
  state branch into v1, `lbu v1,3(s4)`); every path ends `b <tail>` with the
  tail's mode reload (`lbu a1,3(s4)`) duplicated into the branch delay slot
  (7 mutually-exclusive copies — the branch-likely d-slot duplication pattern).
- mode==1: three `FastVecNormalize` passes over the level forward/side/up
  block (`levelCamData.vecBlock`, 0x13F350+0x2080) with f20 as the vector
  accumulator, `Camera_Pos2Polar3d` into base+0x70, then the 128-bit blendWork
  copy (`lq/sq`) and `func_002144D8`.
- Tail (common): mode reload (a1, from d-slots); `lui v1,0x18; li v0,3;
  sh v0,0x71b0(v1)` (state=3); `move a0,a1; bnez a0,` else; `<d: sb a1,2(s4)>`
  (type=mode).
  - mode==0: cluster base `v1 = s4+0x10`; interleaved 32-bit latches
    (posInterp/quatInterp = req*Add, field_1C=field_10=0) with the two 128-bit
    pose copies (pose0=activeCam0, pose1=activeCam1) where each copy
    MATERIALIZES separate base registers (`addiu a1,s4,64; addiu a0,s4,80;
    lq v0,0(a0); sq v0,0(a1)`) instead of offset addressing.
  - else: polar base `s0 = s4+0x70` (computed in the else branch, not
    hoisted); `reqInterpFrames` increment + `func_001F96F8` + `func_001FA6C0`
    + `1.0f /` reciprocal into blendStep/blendStepInv.

## What matches (best candidate, cand10: 592/596, `-fno-gcse`)

- Prologue, frame (0xB0, no s6), s4 base setup: byte-for-byte.
- mode==0 dispatch 128-bit copy (0x1ec8e8–0x1ec904): byte-for-byte, using the
  pinned-copy form (`CameraQuad* dst/src` + `asm volatile("" : "+r"(p))` +
  `register CameraQuad val asm("$2") = *src; *dst = val;`).
- mode==1 dispatch body (normalize/Pos2Polar3d/blendWork copy): byte-for-byte.
- Tail pose copies: the pinned-copy form forces the original's materialized
  `addiu/lq/sq` 128-bit copy shape (closes most of the 572→592 size gap).

## Residual diffs (all EGC 2.95.2 scheduler/allocator behavior, not controllable
from the source forms tried)

1. **Polar-base hoisting / s4 clobber**: EGC schedules `addiu s4,s4,112`
   (polar base) into the tail `bnezl` delay slot (runs before the cluster
   block), clobbering the invariant s4. The cluster base then computes to
   base+0x80 instead of base+0x10, and the else branch reuses the clobbered s4
   instead of materializing `s0 = s4+0x70`. The original keeps s4 intact and
   puts each derived base in its own register (v1 cluster, s0 polar). Pinning
   the cluster base to $3 (v1) and the polar base to $16 (s0) is partially
   ignored (cluster lands in a1, polar still clobbers s4).
2. **Tail d-slot distribution**: the original duplicates the tail's mode load
   (`lbu a1,3(s4)`) into every path's `b <tail>` delay slot; EGC instead
   duplicates the state-store hi (`lui v0,0x18`). Happens whether the mode is
   read before or after `state = 3`, as a local or a memory read.
3. **Tail register permutation**: original `lui v1,0x18; li v0,3; sh
   v0,0x71b0(v1); move a0,a1; bnez a0` (hi in v1, const in v0, mode moved to
   a0, plain bnez) vs EGC `li v1,3; addiu a2,v0,0x71b0; sh v1,0x71b0(v0);
   lbu v0,3(a2); bnezl v0` (const in v1, hi in v0, base in a2, branch-likely).
4. **128-bit pose-copy register choice**: original uses a0/a1 (dst/src swap
   between the two copies); EGC uses a0/v1 and (for the second copy) clobbers
   s4 for the src base.

## Attempts (working/camera_func_001EC8A0/cand*.cpp)

- cand1 (560): baseline; folded copies, s6 frame 0xC0, wrong regs.
- cand2 (572): pinned dispatch copies + `struct LevelCamData{u8 pad[0x2080];
  u32* vecBlock;}` — the vecBlock load form (`lui s0; addiu; lw a1,8320(s0)`)
  only matches via the struct field (array+cast folds the constant).
- cand3 (572): unpinned copies fold to offset addressing (pins required).
- cand4 (588): volatile `TRANS_REQ_TYPE` macro — dispatch reloads the mode per
  comparison (original CSEs within each state branch); structurally wrong.
- cand5 (572, `-fno-gcse`): frame 0xB0, no s6 (the s6 hi-cache is a GCSE
  decision; `-fno-gcse` is what fixes it). Type store emitted `sb zero` because
  the `requestedType` local is value-propagated to 0 in the `==0` branch.
- cand6 (broken): cluster/polar bases derived from the direct `&camTransState`
  symbol — EGC computed the cluster relative to the polar base (wrong offsets).
- cand7 (572, `-fno-gcse`): memory reads (`camTransState.type =
  camTransState.reqType`) fix the type store (registers the mode, not 0), but
  EGC hoists the polar base into the `bnezl` d-slot → cluster base base+0x80.
- cand8: pins — wrong pin ($20 is s4, not s0); retested in cand10 with $16.
- cand9 (`-fno-gcse,-fno-schedule-insns`, 560): disabling the scheduler breaks
  the whole function (131 diffs) — the original used the default scheduler.
- cand10 (592, `-fno-gcse`): correct pins (pPos→$3, pPol→$16) + pinned-copy
  materialization of both tail pose copies. Best so far; residual diffs 1–4
  above remain.

Flags: camera.o has no per-TU override; a match would require `-fno-gcse` as a
PRIVATE_COMPILE_FLAGS for camera.o, which recompiles every matched camera
function in the TU (Pos2Polar3d, stage/commitPendingTransform, ExecuteCamPostUpd
neighbors) and would need re-verification. `-fno-cse-follow-jumps` does NOT fix
the s6 issue. cfront rejects `asm volatile("" ::: "memory")` (parse error at
`::`) — plain `asm volatile("")` works.

## Last-resort escalation

last-resort-decompiler (GPT-5.6 Sol) invoked 2026-09-23 for this exact target:
recommended `-fno-gcse` (mechanically verified to fix the s6/frame blocker),
pin pLast→$21/pBlend→$20, a `requestedType` local read before the state store,
and the +0x10/+0x70 sub-views. All applied (cand5/cand10); the s6/frame issue
was fixed, the residual scheduler/allocator diffs (polar hoisting, d-slot
distribution, tail register permutation, pose-copy register choice) remained.

Retain INCLUDE_ASM. See blocked.json entry
`code/game/camera.cpp:func_001EC8A0`.
