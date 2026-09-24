# Camera_OffsetTick (func_001ED360)

- Source: `code/game/camera.cpp` (definition at line 561, declaration with
  `asm("func_001ED360")` at line 559).
- VRAM: `0x001ED360`, size 268 bytes (0x10C).
- Matched: 2026-09-24. Full boot ELF parity passes; count dropped 663 -> 662.
- Flags: camera.o uses the default TU flags (`-G8 -O2 -ffast-math
  -fno-exceptions -snas`); no `PRIVATE_COMPILE_FLAGS` needed.

## Semantics

`void Camera_OffsetTick(CamOffsetRec* p, int which)` advances one axis of the
decaying camera-position oscillation (a "sway" effect). `CamOffsetRec` is a
16-byte per-axis timer passed in by the caller (`FUN_001EDAA8` calls it with
two slots at 0x1870A0/0x1870B0 and `which` 0/1); the slots live in
`currentCamera`'s pad region, so the function only ever touches them through
the pointer (no hardcoded address in source).

```
struct CamOffsetRec { float amp; float result; int total; int elapsed; };
```

If the active camera (`curCam`) is colliding with the hero in mode 6, the
oscillation is cancelled (`total=elapsed=0; return`). Otherwise, while
`total != 0` it clamps `elapsed` up to `total`, decrements the timer with
`FastDecTimer`, computes the offset magnitude
`amp*cos(2*total)*(total/elapsed)^2`, and adds
`normalize(selected axis) * result` to the camera position. `which==0`
selects orientation axis `q[2]`, `which==1` selects `q[0]`. The `else`
branch just clears `elapsed`.

## Helpers (all in the generated fast-function region, `code/_generated/game/fastfunc.s`)

- `extern "C" float func_001FA6C0(int)` — int -> float (declared in `actuator.h`).
- `extern "C" float FastCos(float)` — fast cosine (`actuator.h`).
- `extern "C" float FastNormalizeAngle(float)` — wraps an angle to its
  principal range (added to `actuator.h` for this match).
- `int FastDecTimer(int& x)` — C++ linkage, mangles to `FastDecTimer__FRi`;
  decrements `*x`, returns a 0/1/2 phase (added to `actuator.h` for this
  match). `camera.cpp` now `#include "actuator.h"` for these.

## Codegen notes (what made it match)

- **`if (p->total != 0) { ... } else { p->elapsed = 0; }`, not an early
  return.** Writing the positive path first with the `elapsed=0` clear in the
  `else` reproduces the original's branch layout. An `if (total == 0) { ...;
  return; }` early-exit inverted the branch direction (EGC emitted an extra
  `b` / different delay-slot schedule). This was the last-resort-decompiler's
  fix and it resolved the only remaining 49-word diff.
- **Float block is 3 `i2f` calls** (no dead call): the numerator `total`, the
  `elapsed` denominator, and the `total` for the angle. `totalF + totalF`
  CSEs to `f0+f0`. The byte-exact order is:
  `ratio = i2f(total)/i2f(elapsed); totalF = i2f(total);
  ang = FastNormalizeAngle(totalF+totalF); c = FastCos(ang);` then the
  multiply chain `v=amp; v*=c; v*=ratio; r=v*ratio; result=r;`.
- **Reset-block store order** is the reverse two-constant-store rule: source
  `p->total = 0; p->elapsed = 0;` emits `sw elapsed` before the branch and
  `sw total` in the delay slot.
- **`curCam` must be `extern u32 curCam __attribute__((section(".data")));`**
  (already the case in camera.cpp); without `.data` the absolute self-based
  load scheduling does not match.
- `beqzl` in the reference tests `total == 0` (branch-if-equal, likely) — do
  not read it as a `<=` test.

## Verification

- Standalone probe (`tools/decomp_probe.py` on `working/func_001ED360/cand12.cpp`
  with `--define func_001FA6C0=0x1fa6c0`): 268/268, 0 word diffs.
- Production: clean `make` + `cmp build/boot_elf.elf assets/boot_elf.elf`
  byte-for-byte.

## Escalation

`last-resort-decompiler` (usage-limited) was invoked for this exact target and
returned the `if (total != 0) {...} else {...}` structure above; testing it
produced the byte-exact match, so no `blocked.json` entry is needed.
