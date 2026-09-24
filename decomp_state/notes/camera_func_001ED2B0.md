# Camera_BlendCams (func_001ED2B0, 0x1ED2B0, 176 bytes)

Matched 2026-09-24. Per-frame camera blend dispatch (Deadlocked twin:
`void Camera_BlendCams(UpdateCam *pCurr)`): steps the active camera toward
the target via `Camera_TransitionStep` (pos/quat, camTransState+0x10) when
`camTransState.type == 0`, else via the polar step `func_001ECCD8`
(camTransState+0x70). When a step reports done and `occlCamState.staged == 0`,
commits `pTgt->mtx0/mtx1/mtx2` to `currentCamera.orientMtx[0..2]`
(0x187290) and `pTgt->posQuad` to `currentCamera.pos` (0x187080), then
clears `camTransState.state` and `.type`.

## Structure (original)

- Prologue: `addiu sp,-48; lui v1,0x18; sq s1; sq s0; addiu s1,v1,0x71B0
  (pBlend); sq ra; lbu v0,2(s1)` (type).
- Dispatch: `bnez v0 → polar`; normal in FALL-THROUGH (a0 = incoming pTarget
  reused, no setup); polar in TARGET with explicit `move a0, s0` before its
  jal. The pTgt init `move s0,a0` sits in the bnez delay slot. EGC keeps the
  THEN branch in the fall-through (tested both polarities; a polar-first if
  flips the layout to `beqz` + polar fall-through, which does not match).
- Done test: `beqz v0 → epilogue`; ds = `lui v0,0x19`.
- Staged: `lw v1,-15572(v0)` (occlCamStaged, 0x18C32C; base v0, value v1);
  `bnel v1 → 1ed348 (sh state=0)`; ds = `sb zero,2(s1)` (type=0, DUPLICATED
  — see merge below).
- Copy block: base in v1 (`lui v1,0x18; addiu v1,v1,0x7290` = orientMtx);
  per-copy `[lq v0,0(src); sq v0,0(dst); addiu dst_next; addiu src_next]`;
  d1→a1, d2→a2, d3 = base-0x210 reusing v1; src in a0 (reused); value v0.
- Merge: `[sb zero,2(s1) (type), sh zero,0(s1) (state)]`, bnel target = the
  sh (state).

## Matching forms (EGC 2.95.2, default flags)

1. **Polar a0 setup**: `asm volatile("" : "+r"(pTgt));` (tied read/write
   zero-byte barrier) inside the else arm before the polar call. Without it
   EGC reuses the incoming a0 on both paths (it tracks a0 == pTarget == pTgt
   through the ds init). With it, EGC emits `move a0,s0` only on the polar
   path. A normal-first if with both args = pTgt reproduces the bnez layout
   (normal fall-through, polar target); passing the raw `pTarget` to the
   normal call instead breaks the prologue (EGC routes pTarget through v1:
   `move v1,a0` + base to v0).
2. **Merge store order**: source `pBlend->state = 0; pBlend->type = 0;`
   (state FIRST). EGC's two-merge-store algorithm (verified with three
   variants): source `[M1, M2]` → ds = M2 (duplicated), tail = `[M2, M1]`,
   bnel → M1. Original = ds type + tail [type, state] + bnel → state, so
   M1 = state, M2 = type. A block-tail `type = 0` inside the staged if plus
   merge `[type, state]` (3 stores) gives the wrong layout (ds = state,
   trailing dup); merge `[type, state]` alone (2 stores, no block tail) gives
   ds = state. Only `[state, type]` matches.
3. **128-bit copy block**: same pattern as the camera 128-bit-copy family —
   `register CameraQuad* d0 asm("$3")` + `"+r"` barrier (defeats offset
   folding), `register CameraQuad value asm("$2")`, per-copy `"+r"` barriers
   on the derived dst/src pointers, bare `asm volatile("")` boundary after
   each sq (stops the next dst/src addiu hoisting into the lq/sq gap),
   `register CameraQuad* d2 asm("$6")` (a2; unpinned d2 reuses a1),
   `register CameraQuad* s3 asm("$4")` (a0; without the barrier src3 folds
   to `lq v0,48(s0)`). d3 stays RELATIVE: `(u8*)d0 - 0x210` — the original
   computes it `addiu v1,v1,-0x210` from the orientMtx base; an absolute
   `&currentCamera.pos` changes the codegen. (d3 target = currentCamera.pos,
   0x187080 = orientMtx - 0x210.)
4. **Staged load**: `occlCamState.staged` (struct field, 20-byte struct →
   absolute access) matches; the equivalent plain `occlCamStaged` u32 global
   also matches but needs `__attribute__((section(".data")))` (out of the gp
   window; -G8 would classify it .sdata and fail the probe link).

## Verification

- `tools/decomp_probe.py` (candidate10 form): 176/176 bytes, 0 differences.
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf`: pass.

## Dead ends

- Polar-first if (`if (type != 0) polar else normal`): EGC emits `beqz` with
  polar in the fall-through — wrong layout.
- Staged pin (`register u32 staged asm("$3") = occlCamStaged;`): produces the
  correct staged regs (base v0, value v1, bnel v1) but corrupts the prologue
  (`move v1,a0`, base to v0, pTgt init via v1). Not used.
- Normal call with raw `pTarget` (instead of pTgt): prologue corruption as
  above, and still no polar `move a0,s0`.
- Inner block-tail `type = 0` + merge `[type, state]` (v6): ds = state (dup),
  trailing sb dup, bnel → trailing sb — wrong.
