# func_001ED470 — Camera_updateCollState (BLOCKED)

Per-frame camera-collision update, 0x1ED470, 0x380 bytes (224 words), frame 208.
Investigated and named `Camera_updateCollState`; the C form is fully understood
and semantically correct but EGC 2.95.2 does not reproduce the instruction
ordering, so the function is retained as `INCLUDE_ASM`.

## What the function does

Reads `levelCamData` (0x13F350) and `camCollState` (0x1870D0):

1. `FastVecNormalize(&a, &levelCamData.inner.dir290, -1.0f)` into a stack vec.
2. `aPrev = aCur; aCur = a;` (16-byte quad copy through the stack vec).
3. `d = FastVecDot(&dir20, &a)`; if `d < -0.98f`, add `0.2f` to each of
   `a.x/a.y/a.z`.
4. Smooth `dir20.xyz` toward `a.xyz` with
   `Cam_InterpValues(cur, a, &off50/54/58, 0.015f, 0.2f, 0.0f)`, then
   `FastVecNormalize(&dir20, &dir20, 1.0f)`.
5. `v70 = dir80 - v60` (`FastVecSub`), `FastVecLength(&v70)`,
   `d = FastVecDot(&v70, &a)`; cache `fA0 = fA8 = d`.
   `t = FastVecNormalize(&a, d)`; `b = t` (quad copy);
   `v80 = v70 - t`; `fA4 = FastVecLength(&v80)`;
   `FastVecScale(&v80, &v80, 1.0f/fA4)`; `v60 = dir80` (quad copy).
6. If `i2284 != 0x50 || i2084 == 0x11`: `f00=dir80[0]; f04=dir80[1];`
   `f08 = Cam_InterpValues(f08, dir80[2], &off10, 0.0075f, 0.175f, 0.0f);`
   `f0C = dir80[2];` else just `f00=dir80[0]; f04=dir80[1];`.
7. Shift the 5-slot `f98` ring right by one (`ring[i]=ring[i+1]`, i=0..3) and
   `ring[4] = f98`.
8. Moby tracking on `p = pCollMoby` (`MobyInstance*`):
   - if `p == 0 || oClass == 0x4BA || oClass == 0x336`:
     `mobyZDelta = 0; pCollMoby = 0; lastMobyZ = dir80[2];`
   - else if `ccs.pCollMoby == p`:
     `mobyZDelta = FastAbsF(p->pos.z - lastMobyZ);` if `>= 0.001f` then
     `mobyZDelta = 0;` `lastMobyZ = ((Moby*)ccs.pCollMoby)->pos.z;`
   - else: `mobyZDelta = 0; pCollMoby = p; lastMobyZ = p->pos.z;`

## Verified data layout

`CamCollState` (0x1870D0, 0xE0 bytes): `dir20@0x20 Vec4`, `aCur@0x30`
(CameraQuad), `aPrev@0x40`, `off50/54/58@0x50/54/58 f32`, `v60@0x60`
(CameraQuad), `v70@0x70`, `v80@0x80`, `b@0x90` (CameraQuad),
`fA0@0xA0 f32`, `fA4@0xA4`, `fA8@0xA8`, `ring[5]@0xAC f32`,
`pCollMoby@0xD4 u32`, `lastMobyZ@0xD8 f32`, `mobyZDelta@0xDC f32`,
`off10@0x10 f32` (inside the dir20 quad).

`LevelCamData` (0x13F350): inner block at +0x20; `dir80[4]` at
levelCamData+0x80 (0x13F3D0), `f98` at +0x98, `dir290[4]` at +0x290
(0x13F5E0), `pCollMoby` at +0x2FC, `i2084` at +0x2084, `i2284` at +0x2284.
Anchor relations (used by the s6 form): `&dir80 = &dir290 - 0x210`,
`levelCamData.base = &dir290 - 0x290`.

`MobyInstance`: `oClass` short at +0xA6, `pos.z` f32 at +0x18.
Constants: -0.98f / 0.2f / 0.015f / 0.2f / 0.0075f / 0.175f / 0.001f.

## Original register plan (from objdump of assets/boot_elf.elf)

- `s5` = &camCollState (saved; `addiu s5,s7,lo` where `s7=%hi(camCollState)`).
- `s7` = %hi(camCollState) (saved).
- `s6` = &levelCamData.inner.dir290 (saved; `lui v0,0x14; addiu s6,v0,lo`).
- `s0` = s5+32 = &dir20 (set AFTER the first FastVecNormalize, at word 29).
- `s2` = s5+112 = &v70, `s3` = s5+96 = &v60 (derived from s5).
- `s4` = &dir80 first, then RE-BASED to `s6-656` = levelCamData.base for the
  i2284/i2084/dir80/f98/pCollMoby block.
- `s1` = sp+16 = &t (the v70-block scratch quad).
- Prologue interleaves the address setup (s6 at word 5, s7/s5 at 8/12) with
  the saves, and the first `jal FastVecNormalize` is word 21 with the `swc1
  $f20` save hoisted into its delay slot (word 22).
- Ring loop is COUNT-UP: `a2=&ring`, `a1=i` (0..4), `a0` moving base
  (`addiu a0,a0,4` in the `bnez` delay slot), `a3=%hi` reused to load `f98`
  (a3+152) and `pCollMoby` (a3+764) after the loop.
- Moby tail: `a0=p`, `v1=oClass`; `lh v1,166(a0)`; `li 0x4BA; beq`; `li 822;
  beql`.

## Attempts (all EGC 2.95.2, -G8 -O2 -ffast-math -fno-exceptions -snas)

Metric: SequenceMatcher over mnemonics with immediates normalized (diff~),
orig 224 words / frame 208.

- **State A** (`ccs` pinned $21, `lcd` pinned $20, all accesses via `ccs->` /
  `lcd->inner.`): 212 words, frame 192, diff~ 220. Correct camCollState
  derived structure (s5=ccs, s0=s5+32, s2=s5+112, s3=s5+96) but %hi(ccs) is a
  prologue TEMPORARY (not saved) and &dir290 is computed inline, so frame
  stays 192 and s6/s7 are not saved.
- **State B** (pinned field pointers `p20`/`p70`/`p60` to $16/$18/$19 + tied
  barriers, direct `camCollState` accesses): 224 words, frame 208, diff~ 135
  (lowest). BUT the full camCollState base lands in s4, not s5 — the original
  keeps it in s5 (`sq s5,112(sp)` + `addiu s5,s7,lo`), so State B can never be
  a byte match.
- **S6 anchor** (last-resort recommendation): `ccs` $21 (s5),
  `levelAnchor=(u8*)&levelCamData.inner.dir290` $22 (s6), then
  `levelCursor=levelAnchor-0x210` $20 (s4=&dir80) for the v70 block / 128-bit
  v60 copy, re-based to `levelAnchor-0x290` (levelCamData.base) for the
  conditional/moby block; `p20` $16 declared after the first normalize; ring
  loop as do-while/for with `ringBase` $6 + running pointer; moby `mp` pinned
  $4. Result: 211 words, **frame 208 (matches)**, **correct register
  structure** (s5=ccs saved, s7=%hi saved, s6=&dir290 saved, s0/s2/s3 derived
  from s5, s0 set after the first normalize), diff~ 206.

The S6 anchor is the right foundation (only form with the correct saved
register set + frame) but diff~ 206 means ~64/224 words match in position.

## Remaining diffs (S6 anchor vs original) — all EGC scheduler/allocator

1. **Prologue** (words 1-29): original interleaves s6/s7/s5 address setup with
   the saves and puts the first `jal` at word 21 with `swc1 $f20` in its delay
   slot; EGC emits all saves as a block first, then the setup, then the `jal`
   with f20 already saved. The aPrev/aCur copy also differs: original
   materializes addresses (`addiu a0,s5,64; addiu v1,s5,48; lq v0,0(v1);
   sq v0,0(a0)`), EGC uses direct `s5+offset`.
2. **v70 block**: original `s1=&t` / `s0=&v80`; EGC `s0=&t` / `s1=&v80` (swap).
   Original keeps `s4=&dir80` as a register for the `FastVecSub` arg; EGC
   computes `addiu a1,s6,-528`.
3. **Ring loop**: original count-up (`a1` 0..4, `a0` moving, `bnez`, `a3` hi
   for the f98/pCollMoby loads); EGC emits count-down (`li v1,3; bgez`) with
   `v0` moving and the f98/pCollMoby loads through `s4`. Both the do-while and
   for-loop C forms produce the count-down — the loop direction and the a3-vs-s4
   hi register are scheduler/RA choices.
4. **Moby tail**: original keeps `a0=p` and loads `oClass` into `v1`; EGC moves
   `p` to `v1` for the branch and loads `oClass` into `a1` (the `$4` pin is
   honored for the initial `lw a0,764(s4)` but not retained through the
   branch).

## Exhausted

- Register pinning: ccs/levelAnchor/levelCursor/p20/p70/p60/ringBase/mp to
  s5/s6/s4/s0/s2/s3/a2/a0.
- Tied read/write empty-asm barriers (`asm volatile("":"+r"(p))`) for p20/p70/p60.
- S6 anchor + levelCursor re-base (last-resort recommendation) — applied.
- Ring loop as do-while AND for-loop (both count-down).
- Moby `mp` pinned to a0.
- Pointer vs array indexing, `CameraQuad` 128-bit copy vs per-float.
- Scheduler flags are not applicable: the original was built with the DEFAULT
  scheduler, and `-fno-schedule-insns[2]` / `-mno-split-addresses` make the
  already-matched camera 128-bit-copy siblings worse (see
  notes/camera_func_001EC710.md, notes/camera_func_001EC8A0.md).

## Escalations (this exact target)

- `expert` (GPT-6 Astra): one-shot consultation used in the prior session
  (see working/camera_func_001ED470/expert_dossier.md).
- `last-resort-decompiler` (GPT-5.6 Sol): invoked with the full dossier
  (original asm, Ghidra, State A/B metrics, object diffs). It (a) wrongly
  claimed fA0=length and mobyZDelta=signed — both disproved by objdump
  (fA0 is the FastVecDot result; mobyZDelta stores the FastAbsF result);
  (b) correctly confirmed all null/excluded-class moby paths zero mobyZDelta
  (applied); (c) recommended the S6 anchor (applied — gives the correct
  structure + frame), the ring-loop a2 base + running pointer (applied —
  count-down either way), and the moby a0 pin (applied — not retained). No
  further concrete route remained.

## Outcome

Reverted to `INCLUDE_ASM` (overlay-safe; the generated assembly references the
named `levelCamData` symbol). Full boot ELF parity preserved
(`make split && make -j2 && cmp build/boot_elf.elf assets/boot_elf.elf` →
byte-for-byte). Shared declarations retained: `LevelCamData` /
`levelCamData` (0x13F350), `CamCollState` / `camCollState` (0x1870D0, used by
the matched function at camera.cpp:208), `FastVecScale` (0x1F9A68, common.h).
Best C candidate preserved at working/camera_func_001ED470/camera_s6anchor.cpp.
