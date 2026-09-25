# UpdateCamera (func_001EDAA8) — matched 2026-09-25

Address 0x1EDAA8, 0x188 bytes (392 bytes / 98 words). Per-frame single-camera
update. Matched with default camera-TU flags (`-G8 -O2 -ffast-math
-fno-exceptions -snas`); full boot ELF parity passes (cmp clean, count 648).

## What it does

`UpdateCamera()` is the per-frame driver for the active level camera
(currentCamera, 0x186F40). Each frame it:

1. In **vendor mode** (`GameMode == 5`): if a text screen is up
   (`textScreenActive`, 0x1E6400 nonzero) it clears the transition state
   (`camTransState.state = type = 0`) and returns early — the level camera is
   frozen while the vendor text screen owns the screen. Otherwise it falls
   through to the normal update.
2. Bumps `currentCamera.camTimer`.
3. `Camera_HandleScreenFade()` (func_001EDA60) — the screen-fade step.
4. `Camera_updateCollMode()` (func_001ED940) — collision-mode refresh.
5. `func_001ED470()` — blocked sibling (0x1ED470); called unconditionally.
6. `UpdateAllCameras()` (UpdateAllCameras__Fi, 0x1EC420) — multi-camera update;
   the boot call is a bare `jal; nop` with **no a0 setup** (see Codegen notes).
7. If the blender state is in the "stepping" range
   (`(unsigned short)(state - 1) < 2`), calls `func_001EC8A0(pLastUpdCam)`.
8. If the blender state == 3, `Camera_BlendCams(pCurr)`; **else** if the
   occlusion subsystem has not staged its own transform
   (`occlCamState.staged == 0`), commits the current UpdateCam's orientation
   matrix quads + position quad into currentCamera (the 128-bit copy block).
9. If `occlCamState.staged == 0` again, decomposes the committed matrix into a
   polar via two callees into a stack `CameraMatrix work`.
10. Ticks the two offset records (`Camera_OffsetTick` x2), runs the collision
    check `func_001ED7F0()`, the zone lookup `func_001EE4B0(&pos)`, and (when
    `orientRebuildFlag`, 0x15EDB4, is set) `FastVecCross` re-derives the third
    orientMtx axis from the other two.

Deadlocked names the equivalent `UpdateCamera` (reference/dl/game_dl/camera.cpp
~4304); RC1 keeps the ancestor single-camera form.

## Naming / symbols

- `UpdateCamera__Fv = 0x001EDAA8;` added to config/symbols.txt (was Splat
  placeholder func_001EDAA8). The C definition is a plain C++ `void
  UpdateCamera(void)` — cfront mangles it to `UpdateCamera__Fv` with no asm
  override needed. The symbol lands in camera.o at the pinned address; the
  stale generated func_001EDAA8.s is not linked.
- `textScreenActive = 0x001E6400;` — vendor text-screen active flag (int).
- `orientRebuildFlag = 0x0015EDB4;` — orientation-rebuild flag (u8).
- `#define GAME_MODE_VENDOR 5` (local to camera.cpp): matches bmain.cpp's
  GAME_MODE family and Deadlocked mode.h (GAME_MODE_VENDOR 5).
- Struct carving in code/include/camera.h: the `Camera.pad_14C[0x34]` block was
  split into `pad_14C[4]`, `PolarSm rot` (0x150), `pad_15C[4]`,
  `CamOffsetRec offset0` (0x160), `CamOffsetRec offset1` (0x170).
  `CamOffsetRec { float amp; float result; int total; int elapsed; }` moved to
  camera.h (was a local in camera.cpp).
- `FastVecCross` (handwritten, game/fastfunc) declared in code/include/common.h
  next to the other FastVec* helpers.

## Codegen notes

- **func_001ED7F0 takes ZERO arguments.** This was the last blocker (v13 → v16).
  The original tail leaves `$a0` stale (the offset1 pointer from the prior
  `Camera_OffsetTick`) and the call is a bare `jal; nop` with no a0 setup.
  Declaring the callee `void func_001ED7F0(CamOffsetRec*)` made EGC *set up* a0
  (an intermediate s1 + a redundant `move a0,s1` → 396 bytes, 4-word diff).
  Declaring it `void func_001ED7F0(void)` and calling `func_001ED7F0();` leaves
  a0 stale exactly as the original. (Disasm confirms func_001ED7F0 overwrites
  `$a0` on entry at 0x1ED80C `addiu a0,zero,6` and reads via `$a1`/`$v1`; it
  never consumes the incoming a0.) See decomp_state/notes/camera_func_001ED7F0.md.
- **textScreenActive two-register load.** The original loads it as
  `lui $2,%hi; lw $3,%lo($2)` (base in one reg, value in another) then
  `bnez`. A plain or named C `extern int` emits a single self-based load (base
  == value); a pointer form adds a stray `addiu`. The matching form is a short
  inline-asm pair pinned to `$3` (`=r(tsa)`) so the value lands in a register
  the `if (tsa != 0) return;` test consumes. `volatile` did not split it.
- **128-bit copy block** (the else-if branch): the four orientMtx/pos quad
  copies must emit lq/sq with every field address materialized in its own
  `addiu` (dst in a1/a0/a1/a0, src in v1/s1/v1/v1, value in v0). Natural C
  folds the offsets into the lq/sq, so each copy pins its pointer/value to a
  disjoint register with a tied `"+r"` barrier and a zero-byte `asm("")`
  barrier between copies — the same recipe as Camera_BlendCams
  (camera_func_001ED2B0.md / camera_func_001EC710.md). The copy block is the
  `else` of `state == 3`, guarded by `occlCamState.staged == 0`.
- **Work block** (decompose): `CameraMatrix work;` on the stack; the two callees
  take `&currentCamera.orientMtx` and `&currentCamera.rot` as *fields* (not a
  shared base pointer — a shared `pBase` broke the s0/offset scheduling).
  s0 = 0x187290 = &orientMtx, rot = s0 − 0x200.
- **Tail base pointer is NOT pinned.** `CamOffsetRec* pOff =
  &currentCamera.offset0;` left un-pinned gives the original's scratch form
  (`lui v0,%hi; addiu s0,v0,%lo`) with the base in the right place. Pinning it
  to a callee-save reg (`asm("$16")`) flipped it to the direct-lui form and
  mis-allocated the base (v7/v12); keeping `pCurr` live across the tail with an
  `asm(""::"r")` barrier grew the frame 0x70→0x80 (v15).
- Relocations of the matched object: HI16/LO16 currentCamera, HI16/LO16
  textScreenActive, HI16/LO16 occlCamState, GPREL16 orientRebuildFlag, and the
  camera-TU GP-window globals.

## Verification

`make` + `cmp build/boot_elf.elf assets/boot_elf.elf` → clean. Probe history in
working/camera_func_001EDAA8 (v7 copy-block baseline, v12 textScreen asm, v13
non-pinned tail base → scratch-lui form, v16 = v13 + zero-arg func_001ED7F0 →
0/392). last-resort-decompiler was consulted and its zero-arg recommendation
produced the match.
