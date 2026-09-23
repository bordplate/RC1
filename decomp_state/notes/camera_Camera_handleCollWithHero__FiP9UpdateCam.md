# Camera_handleCollWithHero__FiP9UpdateCam (0x1EBE68, 96 bytes) — MATCHED

`code/game/camera.cpp`. Signature `(int, UpdateCam*)`; cfront-mangled to
`Camera_handleCollWithHero__FiP9UpdateCam`. The first parameter is declared
`int` only so the symbol mangles to `(int, ...)`; at runtime it holds an
`UpdateCam*` (a camera entry) and is used as a pointer base. The second
parameter (`pCam`, an `UpdateCam*`) is unused in the body.

## Semantics

Gate for the per-camera hero-collision moby:

```c
s16 mode = *(s16 *)((long)camPtr + CAM_COLL_MODE_OFF); // 0x86
if (mode == 0) {
    if (camCollState.pCamColl == 0)
        camCollState.pCamColl = func_001E9448((vec4 *)((char *)&camCollState - CAM_POS_BACK_OFF));
} else if (camCollState.pCamColl != 0) {
    DeleteMoby(camCollState.pCamColl);
    camCollState.pCamColl = 0;
}
```

- **Camera mode** (`CAM_COLL_MODE_OFF = 0x86`): a 16-bit field read from the
  camera entry in `a0`, six bytes before `UpdateCam::camType` (0x8C). Mode 0
  means "collision active" (spawn the moby); any other value means clear it.
  The Deadlocked reference (`reference/dl/game_dl/camera.cpp`) reads the
  equivalent camera `type` for the same decision, confirming the role; the RC1
  field name is unconfirmed (it is in the `pad_00` region of the current
  `UpdateCam`).
- **State block** (`camCollState`, new symbol at 0x1870D0): holds the current
  collision moby pointer `pCamColl` at +0xC4 (0x187194, the `GameCamera`
  symbol). Moby instances live below 0x10000000 (`MobyInstanceEnd` = 0x15ff1c),
  so the pointer is stored/loaded/compared as a 32-bit value (`lw`/`sw`/`bnez`),
  which is what the original emits.
- **Spawn** (`func_001E9448`, boot-ELF stub, `jr $ra`): the hero-collision moby
  spawn; its real symbol/linkage is unknown (overlay-provided), referenced here
  through the unmangled Splat placeholder via an `asm("...")` override. Its
  argument is the camera position, computed as the hoisted base minus
  `CAM_POS_BACK_OFF = 0x50` (i.e. the `Camera` global at 0x187080).
- **Delete** (`DeleteMoby`, 0x20C828, unmangled symbol): deletes the moby.
  Pinned with an `asm("DeleteMoby")` override (a C++ free function would mangle
  differently).

## Codegen (why the source looks the way it does)

EGC 2.95.2 hoists a single base register for `camCollState` (0x1870D0) and
reuses it for both `pCamColl` (`addiu/lw/sw` at `+0xC4`) and the spawn position
(`addiu $4, $16, -0x50`). Modeling the block as a single struct starting at the
camera position (0x187080) makes EGC pick the top base (wrong `%hi`); a separate
`camCollState` base plus a `(char *)base - 0x50` pointer expression is the only
form that reproduces the original's base register and the `-0x50` offset. The
two offsets are named constants (not raw magic numbers) per the style guide;
both are structural offsets relative to named symbols, not static data
addresses.

## Verification

- 24/24 instruction words match the original (objdump of `camera.o` vs the
  reference `.s`); relocations resolve: `camCollState` -> 0x1870D0,
  `func_001E9448` -> 0x1E9448, `DeleteMoby` -> 0x20C828.
- Full boot parity passes: `cmp build/boot_elf.elf assets/boot_elf.elf`.
- Independently re-verified by the decomp-verifier subagent (clean rebuild +
  `make -B` + parity + function word diff).
