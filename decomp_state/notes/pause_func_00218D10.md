# pause_func_00218D10 (Camera_ResetDefault) — BLOCKED 2026-09-28

`code/game/pause.cpp` (first function in `game/pause` → `pause.o`, default flags).
VRAM 0x218D10, 0x68 bytes (26 words). Retain INCLUDE_ASM; full-ELF parity preserved.

## Semantics (verified vs objdump + Ghidra + the orientMtx reader 0x1F2260)

Resets the camera pose. `currentCamera` is `struct Camera` at 0x186F40.
- `pos`=256.0f, `posY`=256.0f, `posZ`=64.0f (0x187080/84/88).
- Zeroes `orientMtx.q[0..2]` — three 16-byte `sq $zero` stores at 0x187290 /
  0x1872A0 / 0x1872B0 (16-byte stride = CameraQuad stride). The reader 0x1F2260
  uses q[0..2] (48 bytes) as a 3x4 matrix, so the matrix is a 3x4 (12 floats).
- Sets four floats in `orientMtx` to 1.0f at float-indices 0,5,10,11
  (camera+0x350/0x364/0x378/0x37C) = the 3x3 diagonal plus [2][3].

Callers: 0x2192E4 and 0x219730 (both large pause/render state functions).

## Original machine shape (objdump ground truth)

```
lui at,0x4380; mtc1 at,$f1          ; f1=256
lui v0,0x18; lui at,0x4280; mtc1 at,$f0  ; f0=64 (hi interleaved)
addiu v0,v0,0x6f40                  ; v0=&currentCamera
swc1 $f1,324(v0)  addiu v1,v0,0x350 ; posY ; v1=&orientMtx
swc1 $f0,328(v0)  swc1 $f1,320(v0)  ; posZ ; pos
sq $zero,0(v1)                      ; q[0]=0
lui v0,0x18; addiu v0,v0,0x72a0; sq $zero,0(v0)   ; q[1] ABSOLUTE
lui v1,0x18; addiu v1,v1,0x72b0; sq $zero,0(v1)   ; q[2] ABSOLUTE
lui at,0x3f80; mtc1 at,$f0          ; f0=1.0 (DELAYED to here, reuses $f0)
lui v0,0x18; addiu v0,v0,0x6f40     ; RELOAD camera into v0
swc1 $f0,892(v0)                    ; m[11]  (camera+0x37C)
swc1 $f0,848(v0)                    ; m[0]   (camera+0x350)
swc1 $f0,868(v0)                    ; m[5]   (camera+0x364)
jr ra;  swc1 $f0,888(v0)            ; m[10]  (camera+0x378) [delay]
```

## What WORKS (mechanically verified via tools/decomp_probe.py)

Three independent EGC 2.95.2 quirks were each solved; together they take the
candidate from 26 differing words (whole body rescheduled) to **8 words at the
EXACT 0x68 size**:

1. **128-bit zero stores must be inline asm.** Natural C `q[i]=0` (every form
   tried: direct, register-pinned value, struct, array, volatile, barrier)
   lowers to `por $r,$0,$0; sq $r,0(base)` (2 instr) — EGC 2.95.2 materializes
   the 128-bit zero via `por` and NEVER emits `sq $zero`. No matched C function
   in the tree produces `sq $zero`. The original's single-instruction
   `sq $zero` is unreproducible in C here; `asm("sq $zero,0(%0)"::"r"(p))` is
   required (this also fixes the size: 104 vs 112 bytes).
2. **Tied FPR barrier delays the 1.0f load and shares $f0.** `register float
   scalar asm("$f0")=64.0f; ... ; asm volatile("" : "+f"(scalar) : : "memory");
   scalar=1.0f;` forces 64.0 to occupy $f0 through the clears and 1.0 to reuse
   $f0 afterward (the original's exact FPU lifetime). Without it EGC hoists
   1.0 to $f0 at the top and pushes 64.0 to $f2.
3. **Named aliases for q[1]/q[2] give the absolute loads.** `extern CameraQuad
   cameraOrientQ1 asm("D_001872A0")` / `...Q2 asm("D_001872B0")` make EGC
   materialize q[1]/q[2] as absolute `lui 0x18; addiu ,0x72a0/0x72b0` (the
   original's block-B form) instead of `camera+0x360/0x370`.

Best candidate (8 diffs, 0x68 size) = the above + `register Camera* camera
asm("$2")`, `register float large asm("$f1")=256.0f`, `-fno-schedule-insns`,
pos stores in source order posY/posZ/pos, m stores `float* m=(float*)
&currentCamera.orientMtx; m[0,5,10,11]=scalar`:

```cpp
#include "camera.h"
extern CameraQuad cameraOrientQ1 asm("D_001872A0");
extern CameraQuad cameraOrientQ2 asm("D_001872B0");
void Camera_ResetDefault(void) asm("func_00218D10");
void Camera_ResetDefault(void)
{
    register Camera* camera asm("$2") = &currentCamera;
    register float largeValue asm("$f1") = 256.0f;
    register float scalar asm("$f0") = 64.0f;
    camera->posY = largeValue;
    camera->posZ = scalar;
    camera->pos = largeValue;
    { register CameraQuad* q0 asm("$3") = &camera->orientMtx.q[0];
      asm volatile("sq $zero, 0(%0)" : : "r"(q0) : "memory"); }
    { register CameraQuad* q1 asm("$2") = &cameraOrientQ1;
      asm volatile("sq $zero, 0(%0)" : : "r"(q1) : "memory"); }
    { register CameraQuad* q2 asm("$3") = &cameraOrientQ2;
      asm volatile("sq $zero, 0(%0)" : : "r"(q2) : "memory"); }
    asm volatile("" : "+f"(scalar) : : "memory");
    scalar = 1.0f;
    register Camera* outputCamera asm("$2") = &currentCamera;
    asm volatile("" : "+r"(outputCamera));
    float* m = (float*)&outputCamera->orientMtx;
    m[0] = scalar; m[5] = scalar; m[10] = scalar; m[11] = scalar;
}
```
(probed with `--flags=-fno-schedule-insns`; reproduces 8 diffs / 0x68.)

## The 8 residual words = two uncontrollable EGC 2.95.2 RA/scheduler tie-breaks

A. **Camera base hi register (3 words: 0x218D18, 0x218D24, 0x218D5C).**
   Candidate: `lui $4,0x18; addiu $2,$4,0x6f40` (hi cached in $4, reused for
   the block-C reload as `addiu $2,$4,0x6f40`). Original: `lui $2,0x18;
   addiu $2,$2,0x6f40` fresh in BOTH block A and block C (no $4 cache).
   EGC keeps the %hi live across the zero-clears; the original expires it.
   `-mno-split-addresses` makes this WORSE (turns the load into an `la` pseudo
   and adds `move`/`addu`, 26 diffs / 0x6C).

B. **Block-C m-store base + order (5 words: 0x218D60, 64, 68, 6C, 74).**
   Original: m stores off the CAMERA base `$2` (offsets 0x37C,0x350,0x364,0x378)
   in order m[11],m[0],m[5],m[10]. Candidate: EGC introduces a separate
   orientMtx base `$3 = $2+0x350` and stores off `$3` (offsets 0,20,40,44) in
   source order m[0],m[5],m[10],m[11] (m[0] alone stays camera-based).
   The original's m stores are float accesses that EGC kept camera-based;
   no clean C form reproduces that. Nameless unions (a float view of
   CameraMatrix) are a parse error in EGC 2.95.2; a named union would rewrite
   ~10 `orientMtx.q[...]` uses across matched `camera.cpp` functions
   (Camera_TransitionStep, UpdateCamera) — out of scope and match-breaking;
   raw `((float*)camera)[136+i]` offsets are forbidden (static-data-address
   policy).

## Escalations on this exact target

- last-resort-decompiler (GPT-5.6 Sol) invoked 2026-09-28. Recommended:
  `__builtin_memset` for the clears (to select `sq $zero`), the tied `"+f"`
  FPR barrier, distinct named aliases + register pins for q[1]/q[2], and a
  `float* m` for the 1.0 stores. Applied: the `"+f"` barrier and aliases work
  (items 2,3 above); `__builtin_memset` does not compile in EGC 2.95.2 (no
  such builtin; string.h absent) so inline-asm `sq $zero` was substituted; the
  `float* m` form has the same orientMtx-base issue (B). Its stated blocker
  criterion — "if EGC still cannot emit the original address lifetimes/store
  order, the durable blocker is the sched1/reload rematerialization tie-break"
  — is exactly the residual.
- Variants tried (all via decomp_probe.py): natural order; reordered pos;
  exact machine order; `-fno-schedule-insns`; `-fno-schedule-insns2`;
  `-mno-split-addresses`; no camera pins; register pins; nameless union
  (parse error). Best = 8 diffs / exact size.

## Conclusion

Blocked: EGC 2.95.2 SN cannot reproduce the original's (A) expired/reloaded
camera %hi (it caches it in $4) and (B) camera-based block-C float stores
(it introduces a separate orientMtx base and keeps source store order), after
the FPU-lifetime, `sq $zero`, and absolute-address quirks are each forced with
inline asm / a tied `"+f"` barrier / named aliases. Retain INCLUDE_ASM.
