# func_001EE4B0 — Camera_UpdateFog (BLOCKED)

Source: `code/game/effects.cpp` (INCLUDE_ASM placeholder)
Address: 0x1EE4B0, size 0x18C (396 bytes / 99 words)
Caller: `UpdateCamera` in `code/game/camera.cpp`
Semantic name: `Camera_UpdateFog(vec4& pos)` (free C++ function, cfront `Camera_UpdateFog__FR4vec4`)
Blocked: 2026-09-27 — EGC 2.95.2 instruction-scheduling wall.

## Purpose
Updates the level fog for the camera. `pos` is a `vec4` (4 floats) whose low word
is the camera index base. Steps:
1. `r = func_00212C28(&pos, &blend, &idx)` — VU in-fog-box test; returns 0 if no
   fog volume, else sets `blend` (0..1, out via 2nd ptr arg, left on the stack) and
   `idx` (volume index). `if (r==0) return;`
2. `vol = &levelFogVolumes[idx]` — `levelFogVolumes` is an array of `LevelFogVolume`
   (0x80 bytes) at 0x19ADC0. `if ((vol->flags & 2)==0) return;`
3. `w = func_001FA6D0(blend * 255.0f)` (arg in $f12); `inv = 255 - w;`
4. `t = 1.0f - blend;`
5. Four lerps, each `state1*blend + state0*t` (the state1*blend product is the
   accumulator / result register; state0*t is the addend):
   - `fd = farDist1*blend + farDist0*t`
   - `ni = nearIntensity1*blend + nearIntensity0*t`
   - `nd = nearDist1*blend + nearDist0*t`
   - `fi = farIntensity1*blend + farIntensity0*t`
6. Color mix (packed 0x00BBGGRR; low byte→R, mid→G, high→B):
   - `hi1=c1>>16; hi0=c0>>16; mid1=c1>>8; mid0=c0>>8;`
   - `levelFogB = ((hi1&0xFF)*w + (hi0&0xFF)*inv) >> 8`
   - `levelFogR = ((c1&0xFF)*w  + (c0&0xFF)*inv)  >> 8`
   - `levelFogG = ((mid1&0xFF)*w + (mid0&0xFF)*inv)>> 8`
7. Final: `levelFogNearDist=nd*1024.0f; levelFogFarIntensity=fi*1024.0f;
   levelFogNearIntensity=255.0f-ni*255.0f; levelFogFarDist=255.0f-fd*255.0f;`
   Store order: sb B, sb R, sb G, swc1 NearDist, swc1 FarIntensity,
   swc1 NearIntensity, swc1 FarDist.

## LevelFogVolume layout (offsets from element base, kept in $s0)
0x50 int flags (&0x2) · 0x54 int color0 · 0x58 int color1 · 0x5C f32 nearDist0 ·
0x60 f32 nearIntensity0 · 0x64 f32 farIntensity0 · 0x68 f32 farDist0 ·
0x6C f32 nearDist1 · 0x70 f32 nearIntensity1 · 0x74 f32 farIntensity1 ·
0x78 f32 farDist1 · (0x80 total)

## Original register maps (the targets)
FP: $f20=255.0f, $f7=blend, $f12=arg, **$f1=t**; lerp terms farDist0→f4,
farDist1→f2(=fd result), nearInt1→f5(=ni result), nearInt0→f0, nearDist1→f6(=nd
result), nearDist0→f8, farInt1→f3(=fi result), farInt0→f9; 1024.0f→f0 (reused).
INT: w→v0, inv→t1, color1→a1, color0→a2, hi1(=color1>>16)→v1, hi0(=color0>>16)→a3,
mid1(=color1>>8)→a0, mid0(=color0>>8)→t0; B reuses v1, R reuses a1, G reuses a0.

## Where it is stuck
A semantically correct 396-byte candidate (candidate19) reproduces the original's
**99-instruction multiset exactly** and the **complete GPR/FPR allocation** using
verified hard-register locals:
```
register float t  asm("$f1");  register float fd asm("$f2");
register float ni asm("$f5");  register float nd asm("$f6");
register float fi asm("$f3");  register float fd0 asm("$f4");
register float ni0 asm("$f0"); register float nd0 asm("$f8");
register float fi0 asm("$f9");
register int c1  asm("$5");    register int c0  asm("$6");
register int hi1 asm("$3");    register int hi0 asm("$7");
register int mi1 asm("$4");    register int mi0 asm("$8");
```
The **32 residual word diffs are pure instruction scheduling** of independent ops
(identical multiset; prologue, both calls, epilogue, and the final 7 stores all
match):
1. farDist0 vs farDist1 `lwc1` load order (orig loads farDist0 then farDist1).
2. nearDist(nd) vs farIntensity(fi) lerp mul order in the interleaved region
   (orig: nd1*b, nd0*t, fi1*b, fi0*t).
3. The 6 `andi` masks order (orig: mid1, hi1, mid0, hi0, c1lo, c0lo).
4. The 6 `mult` order (orig: hi1, hi0, c1lo, mid1, c0lo, mid0 — interleaved B/R/G).
5. The back-half interleave (fd*255, ni*255, fi*1024, nd*1024, sub.s x2, addu x3,
   sra x3, sb x3, swc1 x4).

## Tried (all default flags -G8 -O2 -ffast-math -fno-exceptions -snas)
- 19 source forms; exhaustive 24 lerp statement orders × 16 per-lerp operand orders
  (576 forms, named + compound) — ZERO reproduced the FP term map without pins.
- Color block before/after the lerp; final-store reorder; color load-order swap;
  nd/fi lerp-order swap.
- expert (GPT-6 Astra, one-shot) recommendation: pin the 5 color intermediates
  (c1/hi1/hi0/mi1/mi0) — reduced 38→32 diffs and fixed the integer map. Its
  boundary `asm volatile("" : "+r"(...))` around the block made it WORSE (56).
- last-resort-decompiler (GPT-5.6 Sol) recommendation: destructive channel
  intermediates with directed zero-byte dependency edges between consecutive ops,
  deferring the nd/fi second terms into the color block (candidate20) — 46 diffs,
  WORSE than candidate19; did not improve it, so per its own criterion the blocker
  is durable. It also confirmed -fno-schedule-insns / -fno-schedule-insns2 are
  worse (both together grow the function 396→404).
- The andi/mult orders in the original (interleaved B/R/G) do not map to any simple
  statement order found.

## Conclusion
Register allocation, instruction selection, semantics, size, prologue, calls,
epilogue, and final stores are all matched; only the priority of independent
operations inside the EGC 2.95.2 default scheduler remains uncontrollable from C
source. Same wall class as `camera_func_001EBF10.md` and `camera_func_001ED7F0.md`.
Retain INCLUDE_ASM. Full boot-ELF parity preserved.

## Probe
```
python3 tools/decomp_probe.py <candidate.cpp> \
  code/_generated/nonmatchings/game/effects/func_001EE4B0.s \
  Camera_UpdateFog__FR4vec4 \
  --define func_00212C28=0x212C28 --define func_001FA6D0=0x1FA6D0 \
  --out /tmp/opencode/fogupdate/vXX
```
