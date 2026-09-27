# UpdateViewContext__Fv (0x1F2D98, 0x61C = 391 words)

Per-frame ViewCtx builder in `code/game/draw_post_post.cpp`. Computes the
fMtx/hMtx camera matrices, the fog ramp, the tfrag setup constants, and the
GIF header + per-pipeline scale-vec blocks; calls FastArcTan/FastCos (4 pairs),
SetTfragDists, and FastMemCopy (x2). Float ABI: 1st arg f12, 2nd f13, result f0.

**Status: BLOCKED (2026-09-28).** ~349/391 words diverge from pervasive EGC
2.95.2 FP/GPR scheduling that source-level control cannot steer. last-resort
GPT-5.6 Sol used. See blocked.json for the summary.

## Verified-matched substructures (by object diff)
These were all achieved and verified, then reverted with the function to
INCLUDE_ASM for a clean tree (they are the resume starting point):

- Frame 0x60; save set exactly `{ra, s0, s1, s2} + {f20, f21, f22, f23}`.
- `k = -8388080.0f` (0xCAFFFBE0) parked in **f23**, loaded in the prologue.
  Forced with `register float k asm("$f23") = VIEWCTX_HMG_K;`. As a `#define`
  or `const` it lands in f22 (0.5 takes f23) — the pin is required.
- The `videoModePal` if/else **diamond** (beqz→NTSC, PAL fall-through,
  b→merge, NTSC) with `hi(viewCtx)` duplicated on both edges. Every ordinary C
  spelling (ternary, initialized local, uninitialized if/else) makes EGC emit a
  1-branch + preload layout. The diamond is forced by anchoring each arm with a
  distinct zero-byte asm:
  ```c
  float f;
  if (videoModePal != 0) { f = 0.756f; asm volatile("" : : "f"(f), "i"(1)); }
  else                   { f = 0.775f; asm volatile("" : : "f"(f), "i"(0)); }
  ```
- f-constant ends up in **f1** and the first viewCtx access goes via **v1**,
  forced by a short-lived scratch pointer used only for the first load/store
  pair:
  ```c
  register ViewCtx* early asm("$3") = &viewCtx;
  asm volatile("" : "+r"(early));
  early->yratio = early->xratio * f;   // only use; never after
  ```
- With the above, words 0-18 (prologue + diamond + merge start) match.

## Semantic corrections found (apply on resume)
1. Combined radial pad `0.5f*(padX/xradpad + padY/yradpad)` is stored at
   **sphereCheckB.z (+0x1E8)**, NOT `.w` (0x1EC). (Original `swc1 $f20,0x1E8`.)
2. `fMtx[14] = nearClip * -2.0f * farClip / (nearClip*(farClip-nearClip)) * k`
   — the constant is **-2.0f** (original `lui 0xC000`), not -1.0f. With -1.0f
   EGC folds `(-1.0f*d)` to a single `neg.s d`, changing the RTL.
   (d=nearClip @+0xA0, fc=farClip @+0xA4, dfd = d*(fc-d).)
3. ViewCtx layout established (for a future camera.h refactor): headerBase0-5
   @0x00-0x5F, headerTex @0x60, headerTexNoFog @0x70, headerGr @0x80,
   headerGrNoFog @0x90, hmgScale @0x180, ihmgScale @0x190, fogVec @0x1A0,
   unSquish @0x1B0, guard @0x1C0, sphereCheckA @0x1D0, sphereCheckB @0x1E0,
   sphereCheckC @0x1F0, fog0 @0x210 (== old perspScale), fog1 @0x214.

## The residual blocker (why it does not match)
Two independent, unmatchable-with-source scheduling behaviors:

1. **Branch-edge hi(viewCtx) register.** The original duplicates
   `lui v0,%hi(viewCtx)` to scratch **v0** on both branch edges, then at the
   merge does `addiu v1,v0,%lo` and `daddu s2,v0,$0` (scratch v0 → v1 full +
   s2 hi copy). My candidate duplicates the page directly into callee-saved
   **s2** and derives s0 from it. A scoped `v0→s2` transfer (register pin +
   zero-byte tied asm) still lands the page in s2; EGC will not keep the
   duplicated load in scratch v0.

2. **128-bit copy CSE (size).** The original materializes dst/src pointers via
   `addiu` immediately before each of the 13 `lq/sq` header/scale copy groups;
   EGC CSEs them, so the candidate compiles **359 words (32 short of 391)**.
   Tied `"+r"` pointer barriers per copy overshoot to **396 words** (5 long)
   without fixing positional alignment; the word diff stays ~349 regardless of
   size in the 359-396 range (the mismatch is in the body, not the tail).

## Failed experiments (do not retry)
- `asm volatile("" : : : "memory")` after the 4 ratio-table copies (phase
  barrier): regressed 349 → 372.
- Tied `"+r"` dst/src barriers on the 13 copy groups: size 359→373→396, diff
  unchanged at 349.
- k as `#define`/`const` (f22 swap); ternary/initialized/uninitialized f
  (1-branch + preload); scheduler flags not run (would need a Splat boundary
  split for this TU and the body mismatch predates scheduling config).

## Tooling
- Word diff: `python3 tools/fdiff.py 0x1F2D98 0x61C` (run from repo root;
  PHDR-based, reads assets/boot_elf.elf and build/boot_elf.elf via PT_LOAD, so
  it works on the section-stripped build ELF).
- Candidate disassembly: objdump of `build/code/game/draw_post_post.o`, extract
  from `<UpdateViewContext__Fv>` to the second `jr`.
- Ground truth: `code/_generated/nonmatchings/game/draw_post_post/UpdateViewContext__Fv.s`.
