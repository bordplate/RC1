# draw occlCamDebugSampler (func_001F0D20) — BLOCKED

Status: blocked (2026-09-25). Retain INCLUDE_ASM; full-ELF parity preserved.

## Target
`code/game/draw_occl.cpp` INCLUDE_ASM `func_001F0CE0` (0x1384) + `func_001F2068` (0x4).
The Splat split (commit 44f983a) carved `game/draw_occl` out of `game/draw`
(boundary 0x1F0CE0 in `config/RC1.yaml`) so the sampler is its own SN TU.

Structure (spimdis `func_001F0CE0,0x1384`):
- Leading dead tail 0x1F0CE0..0x1F0D1F (16 words: `addiu sp,+0x2A0/+0x10/+0x2A0/
  move v0,a2/+0xD0/+0x10/+0x150/+0x60`, each + nop). Ghost — inline asm, not a target.
- REAL function 0x1F0D20..0x1F2063 = 0x1344 bytes = **1233 words** (the decomp target).
- Trailing dead tail 0x1F2068 (1 word: `addiu sp,sp,+0xB0`) + align nop. Ghost.

## What the function is
Debug occlusion-camera sampler/logger state machine (state 0..5, subState 0..10).
DEAD CODE (0 callers, verified by deadness_scan + jal/j scan). Reads occlCamState
(0x18C318) + padState; case 5 writes/reads sample points to
`cdrom0:\DATA\LEVELS\LEVEL<levelId>\occ_sample_deltas.dat;1`. No symbol pinning
needed (no callers) — only emitted bytes must match.

## Reconstruction state (this + prior sessions)
A full C reconstruction exists: `draw_occl_func_001F0D20.candidate.cpp` (wired
form: leading dead-tail asm + C body + trailing tail asm). It **compiles + links**
under `-G8 -O2 -ffast-math -fno-exceptions -snas -mno-split-addresses`. The C
body is `void occlCamDebugSampler() asm("func_001F0D20")`.

Ground truth artifacts: `draw_occl_func_001F0D20.ref.s` (1233-word Splat disasm),
`draw_occl_func_001F0D20.ghidra.c` (flow; stack offsets unreliable).

### Verified codegen facts (probes)
- 24-byte load = `memcpy(char buf[24], (char*)occlSampleDefaults, 24)` → exact
  ldl/ldr/sdl/sdr pattern (probe24).
- `sprintf(buf,fmt,fptodp(x),fptodp(y),fptodp(z))` reproduces the fptodp/sprintf
  register dance exactly (probe_fptodp). `fptodp = u64 fptodp(f32)`.
- `func_001F9B20` returns FLOAT in $f0 → occlCamState+0x8C in the FastArcTan
  `jal` delay slot. `occlSamplePoints` (0x1940E0) is a POINTER scalar.
- 0x4000-branch is LIVE (bnel delay-slot reasoning). state/subState are SIGNED s32.
- Out-of-window scalars need `__attribute__((section(".data")))` (else GPREL16
  truncation). See candidate for the 17.

## THE WALL (why it's blocked) — 2026-09-25 re-verification
All **42 jal calls match** between candidate and reference (sound_update,
SetBackgroundColor, FastVecSub, func_001F9B20, FastArcTan, FastSubRots×2,
func_001FA6C0×2, UpdateViewContext, UpdateFog, and the case-5 sceOpen/Lseek/
Read/Write/Close/sprintf/fptodp/strlen I/O). The reconstruction is
semantically + structurally correct. The residual is a multi-layered
register-allocation + access-pattern wall:

1. **Frame**: ref prologue `addiu sp,sp,-0x150` (frame 0x150); candidate emits
   `-0x160` (one extra live 64-bit value → +0x10). Same s-reg set
   {s0,s1,s2,s3,s4,s5,s7,s8}+ra+f20, but different assignment + local area.
   This first divergence cascades the whole 1233-word body.
2. **padState reload pattern**: the original keeps the padState base HI in s5
   (`lui v1,0x14; move s5,v1`) and **re-derives the base + reloads the
   `pressedButtons` field (0x13CAE4) before ~30 conditional sites** — the
   original source accesses the global field at each site, not a cached local.
   My C caches `s32 padButtons = padState.pressedButtons;` once → EGC keeps it
   in s2, different allocation. Worse, the original's base is **0x13C900**
   (a 0x40-LARGER struct); my `padState` symbol is 0x13C940, so the
   base+`%lo` split differs even for the same absolute field address.
3. **~20 branch-instruction difference** (ref 160 vs cand 140 b*): fine-grained
   condition folding / the 0x4000 live-div layout.
4. **Scheduling** across the whole function (store order, delay slots, hi/lo
   interleave) on top of the above.

Closing the frame requires reproducing the original's exact global access/
aliasing pattern (per-site reloads, 0x13C900 struct base) AND EGC 2.95.2's exact
s-reg assignment for a 1233-word function. Not controllable from clean C in a
bounded number of iterations.

## last-resort (protocol)
- `last-resort-decompiler` (GPT-5.6 Sol) invoked 2026-09-25 (DOSSIER in git
  history / working dir). Verdict: MATCHABLE; corrected two misreads (0x4000
  branch is LIVE not dead; address modes are MIXED GPREL+absolute). Recommended
  strategy implemented: SN-TU Splat split + `-mno-split-addresses` + signed
  s32 state/subState + `.data` absolute scalars + same-address linker aliases
  (`.extern`-seeded) for mixed-mode sites.
- After implementing that strategy the function still does not match (walls 1–4
  above). The SN pipeline / flags are confirmed correct (it compiles + links);
  the residual is pure allocation/scheduling/access-pattern, a compiler
  tie-break not reachable from C source.

## If re-attempting
1. Model the pad block with base 0x13C900 (0x40-larger struct) so the
   base+`%lo` split matches; access `pressedButtons` at each site (no cached
   local) to force the per-site reload.
2. Target the frame: get EGC to 0x150 (one fewer live 64-bit). Pin/reshape.
3. Then chase the ~20 branches + scheduling per-case against
   `draw_occl_func_001F0D20.ref.s`.
4. Keep the C body at the same source position (intra-TU source-order
   addressing). Full `make -j2 && cmp` before committing.
The candidate in `draw_occl_func_001F0D20.candidate.cpp` compiles + links; use
it as the starting point (restore into draw_occl.cpp + Makefile
`$(OBJ_DIR)/game/draw_occl.o: PRIVATE_COMPILE_FLAGS = -mno-split-addresses`).
