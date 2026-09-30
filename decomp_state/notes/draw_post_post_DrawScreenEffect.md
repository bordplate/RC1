# DrawScreenEffect (func_001F4FB8, 0x1f4fb8, 0x180)

Screen VBlank color-effect renderer: background fill + two repeating scanline
bands (A/B). Each band = a GS alpha value (masked `0xFF000000FF`) sent to GS
register `0x42` via VU1_addGSregister, plus a color enabled by its top byte
(`0xFF000000`). An enabled color draws a full-width rect over
`[pos, min(pos+lines, count-1)]` and steps `pos` by `lines`. `count` =
`occlCamParamBase.drawH`, right edge = `drawW`. Dead in the boot ELF (called
from DrawDebugProfiler under the always-false screen-overlay guard).

## Register map (original)
- s0 = pos (init 0), s1 = count (init `lh drawH`), s2 = &occlCamParamBase then
  color-enable mask `0xFF000000` (lui s2,0xff00), s3 = MASK `0xFF000000FF`
  (built `ori/dsll 24/ori`), s4 = base copy (drawW loads in the loop).
- `count-1` is RECOMPUTED per band (`addiu v1,s1,-1`), never hoisted.
- The pointer `screenColorEffectNow` (0x15F350, GPREL) is reloaded before every
  field deref (8 loads) and shared across same-statement field accesses
  (aColor+aLines share one load).

## Match-critical form
Pin `count` to s1 (`register s32 count asm("$17")`) and leave `pos` a plain
local. That keeps all five callee-saved registers live, so:
1. EGC cannot cache the pointer (no free reg) -> it re-reads it GPREL after each
   call and shares a load within a statement (the original's 8 loads).
2. `count-1` has no register to hoist into -> recomputed per band.
3. `pos` being a tracked `=0` local makes EGC emit the loop's first check as a
   single `blez count` (pos=0 => pos<count iff count>0).

Pinning `pos` too broke it: EGC then emits `slt v0,s0,s1; beqz v0` for the
pre-check plus an extra nop (384 -> 392 bytes).

The pointer is NON-volatile (not volatile): volatile over-reloads it (17 loads,
one per `->`), while non-volatile with no free register reloads only after
calls (8 loads). A `.extern screenColorEffectNow, 8` seed (top of the TU) keeps
the bare small-data pseudo expanding to GPREL16 instead of self-based.

## Iterations
- candidate (non-volatile, no pins): 420 B, 94 diffs (pointer cached in s5,
  count-1 hoisted, 6 s-regs, frame 0x70/0x90).
- +volatile: 440 B (17 pointer loads).
- +volatile + pins pos/count: 444 B (still over-reloading).
- non-volatile + pins pos/count: 392 B (frame/map right; slt+beqz pre-check + 1
  nop remain).
- non-volatile + pin count only (pos natural): 384 B, **0 diffs**.

## Naming / support
- `DrawScreenEffect__Fv` (= 0x1f4fb8), `screenColorEffectNow` (= 0x15F350),
  `screenColorEffect` (= 0x18CA40) added to symbols.txt; `ScreenVBEffect`
  struct + externs added to camera.h; `DrawRectOverlay` prototype added.
- Magic values named `VU1_SCREEN_ALPHA_GS_REG` (0x42),
  `SCREEN_EFFECT_ALPHA_MASK` (0xFF000000FFUL), `SCREEN_EFFECT_COLOR_ENABLE`
  (0xFF000000) per STYLEGUIDE.

## Verification
First-try match on the count-pin form: 96/96 words, 0 differences. Independent
decomp-verifier reconstructed the 96 words from both ELFs: 0 differing words.
`make clean && make split && make -j2` + `cmp build/boot_elf.elf
assets/boot_elf.elf` byte-identical; object defines `DrawScreenEffect__Fv`;
decomp_status 614 -> 613.
