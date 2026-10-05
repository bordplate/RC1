# draw DrawDialogText__Fv — wave A-section correction + v2 state (2026-10-05)

Target: `DrawDialogText__Fv` (0x1FBC50, 1139 words) in `code/game/freeze.cpp`.
The 6-mode pause/race-quit dialog. NOT yet matched; this note records the
decisive A-section correction and the remaining (well-understood) residual so a
future session does not re-derive it. Continuation checkpoint lives in
`working/draw_dialog_text/` (NOTES.md, candidate_v2.cpp, authoritative_disasm.txt).

## Headline: the wave A-section "dead FP ops" are LIVE delay-slot call args

The prior plan treated the three FP ops in the A-section as dead and tried to
force them with `+f` asm barriers. That was WRONG. They are the argument
computations for the two helpers, scheduled into the JAL delay slots:

    int period = freezeWavePeriod;
    float phase = (float)(drawFrameCount % period) / func_001FA6C0(period)
                  * 6.28318f;
    int color = FastTweenColor(freezeWaveColorA, freezeWaveColorB,
                               FastSin(phase - 3.14159f) * 0.5f + 0.5f);

Disasm proof (mode-0, lines 601-630 of authoritative_disasm.txt):
  - `jal func_001FA6C0;  lw a0,freezeWavePeriod`  -> arg = freezeWavePeriod.
    Prototype MUST be `extern "C" float func_001FA6C0(int value);` (NOT void).
  - `jal FastSin; sub.s f12,f12,f1`              -> arg = `phase - 3.14159f`.
  - `jal FastTweenColor; add.s f12,f0,f12`       -> arg = `FastSin(..)*0.5f + 0.5f`.
One FastSin call; no FP barriers needed. This supersedes the "stale-a0 works"
and "+f barrier" notes in the working NOTES.md (those produced the 1074-word
v1 with a double FastSin / no-arg helper).

`func_001FA6C0` = intToFastFloat (0x1FA6C0), `func_001FA6D0` = floatToInt
(0x1FA6D0), both in `code/_generated/game/fastfunc.s`. Float args pass in $f12
(standard EGC EE ABI — not a wall; call helpers explicitly).

## v2 candidate measurement (candidate_v2.cpp)

Corrected A-section + f21/f20 clamps + 0x200 mode-3 buffer + faithful-ish mode
bodies. Compiles clean; `.text` = 0x11c0 = **1136 words** vs original **1139**.
(v1 was 1074 — the A-section fix closed 62 of the 67-word gap.) Total size is
within 3 words, but the aligned word diff is still ~0.8% match because the
frame differs — close size + wrong internal structure.

## The real residual: 32-byte frame + incomplete mode-0 body

Frame: mine `addiu sp,sp,-816` (0x330) vs orig `addiu sp,sp,-848` (0x350).
  - Orig prologue saves s0..s8 + ra + **f20,f21,f22** (swc1 at 816/824/832(sp));
    mine saves only ONE fp reg (EGC swaps f20<->f21 by live-range; making
    `slotT` live did NOT add a 3rd save — EGC's fp-save count is not directly
    controllable from C).
  - Orig local area 0x290: buf3[0x200]@sp+0, fWin@sp+0x200, fWin2@sp+0x220,
    buf0[0x40]@sp+0x240, slot@sp+0x280 — the two 24B FontWindows are padded to
    32-byte boundaries and buf0 to 0x240. Mine packs fWin@0x200/fWin2@0x218.
  - 32 B = 16 B (two missing swc1) + 16 B (FontWindow 32B alignment).

The 3 live fp values: f21 = clamped bevel scale `t` (field_0x20*0.125,
[0.1,1.0]); f20 = clamped slot scale `slotT` (field_0x24*0.125, [0.0,1.0]);
f22 = a mode-5 constant (1536.0, the f14/f16 args of func_00200600).

Mode-0 body is INCOMPLETE: orig has ~11 FastTweenColor calls (3 x slotT at
0x1fc708-738 `mov.s f12,f20`, 8 x t at 0x1fc768-890 `mul.s f12,f12,f21`), each
feeding a FontPrintCenter for a colored element. v2 has only 4. rank<3 = bevel
+ 3 prints; rank>=3 = bevel + ordinal-suffix (St/Nd/Rd/Th via sprintf) +
best-time/hi-score stats (div/mult chains %3600, %60, *100) + ~6 prints.

Getting all ~11 colored elements + exact stats right is the main body work; it
also makes f20/f21/f22 simultaneously live, which is what the frame depends on.

## Next steps (in order)

1. Reconstruct full mode-0 (all 3 slotT + 8 t FastTweenColor + FontPrintCenter
   targets; verify rank<3 bevel/prints and rank>=3 ordinal + stats).
2. Re-check the frame (f20/f21/f22 should become live and saved); if the two
   FontWindow locals still pack at 24B, try 32-byte alignment or passing them
   as call arguments to force the original stack layout.
3. Apply the staged data-side carve ONLY with a matching body:
   `tools/patch_freeze_rodata_ld.py` HOLE_END 0x1E78F0 -> 0x1E79A0;
   `config/RC1.yaml` data_suffix 0xe8870/0x1e78f0 -> 0xe8920/0x1e79a0.
   (jtbl values are case-body addresses -> .rodata parity is coupled to body.)
4. Re-measure with the corrected offset and full build + cmp.

## Raw-diff offset note

.text file offset = VMA - 0x1e8d00 + 0xe9c80 = VMA - **0xFF480** (NOT -0xFF400;
an earlier diff read 92 words early). freeze.o .text data at sh_offset 0x1200,
DrawDialogText at +0x198.

## Escalation

`last-resort-decompiler` invoked 2026-10-05: verdict "do not block yet; the
67-word gap is the mistranslated A-section, not an allocator/scheduler wall".
The corrected A-section was tested (v2 = 1136 words). NOT blocked — residual is
understood and addressable; recorded here as a continuation checkpoint.
