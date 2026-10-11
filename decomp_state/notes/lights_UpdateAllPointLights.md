# UpdateAllPointLights (code/game/lights.cpp) — matched 2026-10-11

## Semantics

Rewrites the orbiting background light block `DirLights[13]` from the current
camera radius, then walks the 8 dynamic point-light slots. For each slot whose
`state` is nonzero, if the target `PointLights[i].pos` has moved more than 8.0
from the last synced copy `PointLightData[i].updatePos`, it copies the new pos
over the synced copy and, depending on `state`, creates (1 -> 2) or refreshes
(2) the slot.

```
DirLights[13].color_a = (0.8, 0.8, 0.8, -0.3)          // (x,y,z,w)
phase = FastAddRots(currentCamera.rot.radius, -0.8)
DirLights[13].dir_a.x = FastCos(phase) * 0.866
DirLights[13].dir_a.y = FastSin(phase) * 0.866
DirLights[13].dir_a.z = -0.5
DirLights[13].dir_a.w = 0.0
for i in 0..8:
    if state != 0 and 8.0 < dist(pl.pos, updatePos):
        updatePos = pl.pos
        if state == 1: CreatePointLight(i); state = 2
        elif state == 2: RefreshPointLight(i)
```

The three data tables (named in `config/symbols.txt`, declared in
`code/include/lights.h`):

- `DirLights[16]` @ 0x19BDC0, 16 x 0x40 blocks of two (color,dir) vec4 pairs.
- `PointLights[8]` @ 0x19C1C0, stride 0x20: `color` (0x00), `pos` (0x10), each
  a 16-byte `mode(TI)` quad.
- `PointLightData[8]` @ 0x19C3C0, stride 0x30: three (s16 offset, s16 count)
  pairs, `s16* list` (0x0C), `int state` (0x10), `pad[3]`, `updatePos` quad
  (0x20).

The quads must be `mode(TI)` (16 bytes) so the copy lowers to a single `lq`/`sq`
(the VU reads them with `lqc2`); a plain 16-byte float struct lowers to four
float transfers and breaks the match.

## Codegen findings (the whole battle)

The function body is semantically trivial; the match is a pure EGC 2.95.2
pre-RA scheduling + register-allocation tiebreak in the prologue/last-region
and the loop. Two coupled facts:

1. **Loop anchor.** The original strength-reduces every per-slot access
   (state, the `FastVecDist` argument, and the synced-position copy) onto ONE
   induction register anchored at `updatePos`: state at `-0x10(up)`, copy at
   `0(up)`, arg `= up`, tail `up += 0x30`. To make the compiler derive state
   from `up`, the source takes `state = (int*)(up - 0x10)` (pointer arithmetic
   from the updatePos pointer). A plain `PointLightData[i].field` form instead
   splits into a base chain plus a separate updatePos chain (an extra saved
   GPR, a third tail increment, wrong size 0x170).

2. **Base folding vs the DirLights s0.** With `up` the primary pointer the
   compiler folds the base (`PointLightData + 0x20` as one `lui/addiu`) and,
   because the last `dir_a.w` store is scheduled first, reuses the DirLights
   `s0` for `up` (frame 0x80, 5 GPRs). The original instead loads the two array
   bases as one-time INTERMEDIATES before the `dir_a` stores and keeps the
   `w` store LAST, so `s0` (DirLights) stays live and `up` lands in `s1`
   (frame 0x90, 6 GPRs). The 8.0 threshold must sit in `f22` (not reuse the
   phase's `f20`) so `f20/f21/f22` are all saved.

No natural source form reproduces all of this: every variant either anchors at
the record base (state at `+0x10`, separate arg register, 33 diffs, correct
size) or folds the base and reuses `s0` (correct loop anchor, 0x170 size,
61-83 diffs). Scheduler flags (`-fno-schedule-insns[2]`, single or both) made
it worse (80-94). The match is therefore held with the documented match-
sensitive constructs (STYLEGUIDE "Match-Sensitive Code"):

- Register pins: `up`->`$17`, `pl`->`$18`, `i`->`$19`, the `1`/`2` state
  constants ->`$21`/`$20`, the `8.0` threshold ->`$f22`, and the two array
  bases ->`$3`/`$2` (so they are materialized one-time, not folded).
- `color_a` source order `(w,x,y,z)` and `dir_a` order `(x, y, z, w with the
  pointer setup before y/z and w last)` so each store lands where the original
  schedules it (the original's delay-slot stores hold the PRE-call values).
- A few `asm volatile("" : : : "memory")` barriers pin the branch-likely
  taken-delay `i++` copies and the call delay slots.

The loop keeps the original's branch-likely pattern: a single source `i++`
duplicated into the taken-delay slots of `beql`/`bc1fl`/`bnel` plus one at the
fall-through.

This was resolved by the `last-resort-decompiler` (GPT-5.6 Sol) after the
`expert` (GPT-6 Astra) barrier proposal (`+r` base barrier, do-while) got the
loop anchor right but left the `i` counter in `s0` and the threshold in `f20`.
The Sol form added the `$3/$2` base pins and the `$f22` threshold pin to reach
0 diffs.

## Verification (mechanical)

- `tools/decomp_probe.py` on the standalone body: `original_size` 384,
  `candidate_size` 384, **`match: true`**, 0 differences.
- `tools/tu_assembler_diff.py build/code/game/lights.o build/boot_elf.elf`:
  4 funcs, 4 match, 0 differ.
- Full `make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes
  (byte-for-byte).
- `decomp_status.py --count` 531 -> 530.

## Follow-ups

- `CreatePointLight` (0x00201BA8, 0x3B0) and `DetachPointLight__Fi`
  (0x00201F88, 0x2E8) remain the two big INCLUDE_ASM siblings; both rewrite the
  per-slot 3 link-lists with the 0xF000 detached-tag logic. `RefreshPointLight`
  (wrapper) and `UpdateAllPointLights` are now C.
