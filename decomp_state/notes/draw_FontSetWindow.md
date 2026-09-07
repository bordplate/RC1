# FontSetWindow (vram 0x1F7668, file 0xF85E8, 56 bytes) — MATCHED 2026-09-07

Sets a 12-short font window descriptor consumed by the FontPrintWindow family.
Matched with DEFAULT flags, no special handling.

## Identity / context

`void FontSetWindow(FontWindow *f, short x, short y, short w, short h,
                    short textX, short textY, short lineH, int flags)`

Field meanings read from the consumer 0x1F7090 (reached via 0x1F7580 /
0x1F75F0 wrappers, which only add a palette lookup + D_001DF050/D_001DF3F0
font table pointer):

- [0] x, [1] y, [2] w, [3] h — clip rect (0x1F7090 calls
  func_00233A40(w, h-1, x, y-1) first)
- [4] textX — text draw x (always 0x100 at the call sites)
- [5] textY — initial text y (screen-dependent values, e.g. DAT_0013E504-0x38)
- [6] maxTextH — measured text height (zeroed here, accumulated in 0x1F7090)
- [7] totalH — set to lineCount * lineH in 0x1F7090
- [8] lineH — line advance (0x12 / 0x10 at the call sites)
- [9] flags — bits 1/2/4/8 change width calc, y centering, draw/skip, and
  fractional-offset paths (always 7 at the three call sites)
- [10] offX, [11] offY — fractional offsets (*0.0625) when bit 8 is set

Call sites (all still INCLUDE_ASM): 0x1F4C98 (draw.cpp func_001F4BE0 debug
profiler text), 0x1FEC04 (help.cpp func_001FE980 help display), 0x1FDDEC
(help.cpp func_001FDD58 help init). All three pass the 9th arg as 7 via
`sw, 0(sp)` and the struct pointer as a stack buffer (sp+0x10).

## Codegen findings

Two findings, both confirmed by decomp_probe before integrating:

1. Stack-arg width: the 9th parameter is loaded with `lw $2, 0($sp)` — it is
   a 32-bit (int) parameter in the original source. Declaring it `short`
   makes EGC emit `lh $2, 0($sp)` (1-word diff at 0x1F7668). EGC hoists this
   stack-arg load to the FIRST instruction of the function even though it is
   consumed by the 8th store statement; no source trickery needed.
2. Four trailing constant stores with a register base follow the same
   right-rotation rule as the documented 3-store case: source [S1,S2,S3,S4]
   emits [S4, S1, S2, jr $ra, <S3 in delay slot>]. Verified in v1 probe:
   source order offY,maxTextH,totalH,offX (0x16,0xC,0xE,0x14) emitted
   0x14,0x16,0xC,jr,0xE. Source order maxTextH,totalH,offX,offY (0xC,0xE,
   0x14,0x16) reproduces the original 0x16;0xC;0xE;jr;<0x14> exactly.
   (Generalizes AGENTS.md's 3-store permutation stmt3;stmt1;jr;<stmt2>.)

The first eight stores (the param stores) emit in plain source order; the
7th/8th params arrive in t2/t3 as established for this EGC build.

## Verification

- Probe v2 (decomp_state/probes/fontsetwindow_v2.cpp): match=true, 0
  differences, 56/56 bytes, default flags.
- decomp-verifier independent re-check: probe match, fresh draw.o rebuild,
  function region bytes equal, full cmp pass.
- Clean build + `cmp build/boot_elf.elf assets/boot_elf.elf` pass; count
  748 -> 747.
