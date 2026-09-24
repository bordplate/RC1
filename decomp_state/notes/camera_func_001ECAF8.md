# camera_func_001ECAF8 (Camera_TransitionStep)

Matched byte-for-byte (480 bytes). Source: `code/game/camera.cpp`
(`Camera_TransitionStep`, `asm("func_001ECAF8")`).

## Semantics

Per-frame camera transition step. The caller (func_001ED2B0) invokes it with
`(pTarget, camTransState + 0x10)` when `DAT_001871b2 == 0`.

- `a1` (pB) is a sub-view of `CamBlender` at +0x10: the transition-progress
  scalars (`field_10` quat progress, `field_1C` pos progress) and the
  pose/active-cam quads viewed as float vectors (Vec4).
- Early return `1` when both progress scalars are already 1.0.
- Lerps the active position quad toward the target:
  `activeCam0 = pose0 + (pTarget->posQuad - pose0) * smoothstep(0,1,field_1C)`.
  `pose0` is offset by `camPosOffset` first (FastVecAdd).
- When the occlusion-staged flag is 0 (`occlCamState.staged`, 0x18C32C),
  commits the result: 128-bit copy `activeCam0 -> &currentCamera.pos`
  and `func_001FA2B8(&currentCamera.orientMtx, &work.mat)`.
- Recomputes orientation: `func_002144D8(&work.quat, pTarget)` stages a 128-bit
  quad into sp+0x00, `func_001FA400(smoothstep(field_10), activeCam1, pose1,
  sp)` and `func_001FA4F8(activeCam1, sp+0x10)` write the 64-byte matrix at
  sp+0x10.
- Advances the two clamped progress scalars:
  `field_1C = min(1, field_1C + posInterp * transStepScale)`,
  `field_10 = min(1, field_10 + quatInterp * transStepScale)`.
- Returns 0.

## Key EGC 2.95.2 codegen constraints (the match hinges on these)

1. **pTarget pinned to `$16` (s0), pB left un-pinned.** A plain
   `CamBlendStep* pB = pB_;` lets EGC allocate pB to s1 for *all* accesses
   (early-return + clamp). Pinning pB via `register ... asm("$17")` does NOT
   force s1 for the early accesses (EGC keeps the a1 equivalence for the first
   use) and regresses. So: pin pTarget to s0, leave pB to the allocator.
2. **scale pinned to `$f0`.** `register f32 scale asm("$f0") =
   transStepScale;` is required so the clamp FPU math uses scale=$f0,
   product=$f1, sum=$f2. Without it the clamp allocates wrong FPU regs
   (8-word diff).
3. **No asm barriers.** ANY `asm volatile("")` barrier after the pins
   (input-only, tied `"+r"`, scoped transfer) adds +4 bytes and regresses to
   ~120 diffs. Do not add barriers here.
4. **`CamTransitionWork` aggregate** (CameraQuad quat @ sp+0x00; CameraMatrix
   mat @ sp+0x10) is required instead of separate locals, to get the correct
   call-arg register setup for the EE vector callees (they receive sp and
   sp+0x10).
5. **`currentCamera.orientMtx` (0x187290) declared as a 64-byte
   CameraMatrix** (not u32) to produce the split-address RTL for the
   `func_001FA2B8` call.
6. **128-bit copy to `currentCamera.pos`** (0x187080) uses pinned registers:
   `dst`->`$3`, `src`->`$4`, `value`->`$2`, with a tied `"+r"` barrier on
   dst/src.
7. **`occlCamState` base (0x18C318) is loaded into s3 and +0x14 read** for
   both flag checks; the flag is occlCamStaged (0x18C32C) but the original
   does NOT address it directly — it keeps the base in s3. Using
   occlCamStaged directly changes the codegen.

## Struct changes

- `code/include/camera.h`: added `struct Vec4 { float x,y,z,w; }` (the float4
  view of a CameraQuad for this code).
- `UpdateCam.posQuad` changed CameraQuad -> Vec4 (only referenced at its
  definition; safe).

## Placeholder names resolved (2026-09-24, refactor cleared)

The four placeholders were resolved by research and renamed while preserving
the matched codegen (object diff: only the two/three field-offset immediates
differ and link to the same values; full boot ELF `cmp` passes).

- `D_0015ED60` -> `transStepScale` (f32). First word of the 9-word
  per-video-mode transition param table (0x15ED60..0x15ED80; last word is
  `videoModePal`), written at level start by func_00214970 (mobyutil.cpp
  INCLUDE_ASM, called from transition.cpp func_001E9B10). NTSC 1.0f / PAL
  1.1f.
- `D_0018C318` -> `occlCamState` (struct `OcclCamState`, field
  `staged` @ +0x14 == occlCamStaged 0x18C32C; OcclUpdate @ +0x1C).
  `occlCamStaged` moved from linker_aliases.ld into symbols.txt; the code
  still reads the base + 0x14 (constraint 7).
- `D_00187290` -> `currentCamera.orientMtx` (struct field at 0x350;
  0x186F40+0x350 == 0x187290, no page carry, so `%hi/%lo` are unchanged).
  64-byte orientation matrix, identity-initialized by func_00218D10, read by
  occlusion func_001F2260.
- `camPos16 asm("Camera")` (data symbol 0x187080) removed; the code uses
  `&currentCamera.pos` (0x186F40+0x140 == 0x187080). The standalone `Camera`
  symbol (which collided with `struct Camera`) is deleted from symbols.txt
  and sound.cpp now includes camera.h and uses `(vec4*)&currentCamera.pos`.

Callee placeholders func_002133D0 / func_002144D8 / func_001FA400 /
func_001FA4F8 / func_001FA2B8 remain (separate INCLUDE_ASM, rename when
decompiled).
