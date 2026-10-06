# draw DrawDialogText__Fv — full port done; blocked on EGC global register allocation

Target: `DrawDialogText__Fv` (0x1FBC50, **1141 words / 0x11D4**) in
`code/game/freeze.cpp`. The 6-mode pause / race-quit dialog renderer.
**OUTCOME (2026-10-06): BLOCKED.** A complete C port compiles and matches on
frame, FP saves, the wave A-section, all constants, and the full decode of all
four reachable modes; the sole residual is a global register/address-allocation
difference that EGC 2.95.2 makes internally and that no C lever reproduces.
Reverted to `INCLUDE_ASM`; full boot-ELF parity verified green.

Supersedes the 2026-10-05 "v2 / 67-word A-section" state below (that gap is
closed). Continuation checkpoint + all candidates live in
`working/draw_dialog_text/` (NOTES.md, freeze_ported_v4.cpp,
authoritative_disasm.txt, normdiff.py, structdiff.py).

## What the full port matches (verified against authoritative_disasm.txt)

- **Frame**: `addiu sp,sp,-848` (0x350); saves s0..s8 + ra (10 `sq`) **and**
  f20/f21/f22 (3 `swc1` at 816/824/832(sp)). The 2026-10-05 "32-byte frame"
  gap is CLOSED — once the full mode bodies make f20/f21/f22 simultaneously
  live, EGC saves all three and the FontWindow 32-byte padding appears.
- **Wave A-section** (mode-0): the three "dead FP ops" are LIVE JAL delay-slot
  call args (see below). `func_001FA6C0`=intToFastFloat, `func_001FA6D0`=
  floatToInt. Prototype `extern "C" float func_001FA6C0(int value);`.
    int period = freezeWavePeriod;
    float phase = (float)(drawFrameCount % period) / func_001FA6C0(period)*6.28318f;
    int color = FastTweenColor(freezeWaveColorA, freezeWaveColorB,
                               FastSin(phase - 3.14159f)*0.5f + 0.5f);
- **Constants**: `VU1_addGSregister(71, 0x3004B)` (NOT 0x34B — orig is
  `lui a1,0x3; ori a1,0x4b`); all gp-relative (0x15F4xx/0x15F5xx) vs absolute
  (0x15EE58/5C, 0x15ED80/84) access-mode split; level base lui 0x14 (0x13F350).
- **Modes** fully decoded: case 5 (autosave, VU-text scan + GetIconFrame/
  GetFrameTex + func_00200600), case 2, case 1/default, case 0 (rank<3 bevel +
  3 prints; rank>=3 bevelB + ordinal St/Nd/Rd/Th chain-of-`bne` + TIME section
  + SCORE section). Ref polarity `(currentLevelId^0x10)!=0 ? &RefA : &RefB`;
  time `total = videoModePal ? 3600 : 3000` (movn polarity); time chain uses
  QUOTIENTS (mflo) with two dead mults; score `hs!=0 && hs==hiScore`.
- **Data side**: `.rodata` = 208 bytes EXACT (mFI jtbl 32 + 2 templates 48 +
  jtbl1 32 + jtbl2 96) once the body is present; the staged carve
  (RC1.yaml data_suffix 0xe8920/0x1e79a0; patch HOLE_END 0x1E79A0) is correct
  but is fully coupled to body parity, so it is reverted with the body.

## The wall: EGC global register/address allocation (s7 vs s2)

The `Freeze` global lives at 0x193300 (high page 0x190000 + 0x3300). It is the
most important live value in the function (saved first in the orig prologue).

- **Original**: keeps the HIGH PAGE (0x190000) in **s2** from the prologue
  (`lui v1,0x19; move s2,v1`), then materializes the full pointer per use-group
  (`addiu v1,s2,0x3300` at 0x1FBCCC, 0x1FC1D4, ...) and does field accesses as
  `access field(v1)`. This is EGC's natural far-global codegen (high-page in a
  callee-saved reg + per-use materialization), and it keeps the base allocated
  FIRST (s2).
- **Mine (natural `Freeze.field` access)**: EGC assigns the high page to **s7**
  and folds 0x3300+offset into each access. The base is allocated LATE (s7),
  while `0x70000000` takes s2. This single register-order difference
  (base s7 vs s2) cascades through the 1141-word body: **966/1141 words
  differ**, 1122 words compiled (19 short).

First-write allocation order after the prologue (base = Freeze high page):
  ORIG: s2(base), s3(0x70000000), s5(sp+512), s0(lui 0x1e), s6(21071),
        s4(21066), s1(sp+512 #2), s7(lw gp), s8(lw gp)
  MINE: s7(base), s2(0x70000000), s4(sp+512), s0(lui), s3(extra), s5(21071),
        s6(21066), s1
The original allocates the base FIRST; mine allocates 0x70000000 first and the
base last. That ordering is an EGC-internal liveness/register-pressure decision
not controllable from C without changing the access form.

## Three representations tested (all fail to match; original=1141 words)

1. **Natural** `Freeze.field` global access — high page in s7.
   1122 words, **966/1141** differ (closest). The register is wrong (s7≠s2).
2. **$s2 pin** via a `FreezePage{u8 pad[0x3300]; freeze_t state;}` wrapper +
   `register FreezePage* b asm("$18")` seeded `lui $3,%%hi(Freeze); move %0,$3`,
   accessing `b->state.field` — high page in s2 (correct reg) but EGC FOLDS
   0x3300+offset into each access (`lwc1 $f0,0x3304(s2)`, 1 instr) instead of
   materializing the base (`addiu v1,s2,0x3300; lwc1 $f0,4(v1)`, 2 instr).
   1114 words, 1107/1141 differ.
3. **$s2 pin + tied barrier** on a `freeze_t* f=&b->state; asm volatile("" :
   "+r"(f));` — forces s2 = full pointer (&Freeze, 0x193300) with 1-instr
   `field(s2)` accesses. 1116 words, 973/1141 differ.

The original's specific form (high-page-in-s2 + per-use-group materialization)
is not reproducible: pinning s2 changes EGC's address representation (fold or
full-pointer-in-s2), and in natural access the register choice (s7 vs s2) is
internal. No flag, section attribute, prototype change, or barrier/pin
combination tested reproduces it.

## Escalation

- `last-resort-decompiler` (GPT-5.6 Sol) 2026-10-05 (A-section): "do not block;
  the 67-word gap is the mistranslated A-section, not an allocator wall." The
  corrected A-section was tested and closed that gap (superseded).
- `last-resort-decompiler` (GPT-5.6 Sol) 2026-10-06 (register wall): prescribed
  the $s2 pin (representation 2 above) with check criteria (first save
  `sq $18,688(sp)`, init `lui $3,%hi(Freeze); move $18,$3`, mode-5 scratch
  pointer s2→s3, frame stays 848). Applied and extended with the barrier
  variant (representation 3). Result: the pin DOES move the base to s2 and keeps
  the 848-byte frame, but it changes EGC's address representation (offset
  folding / full-pointer-in-s2) which does not match the original's
  high-page+materialize form, and size regressed (1114/1116 vs 1122). Per the
  recommendation's own "abandon if it folds offsets / adds moves" clause, the
  pin was abandoned after being mechanically diffed. No match.

## Why it is a durable wall

The residual is not missing code (the full port is present and every decoded
section is correct) — it is EGC's choice of (a) which callee-saved register
holds the far-global high page (s7 vs s2) and (b) whether to fold the
0x3300 offset or materialize the base per use-group. Both are internal
allocator/scheduler decisions for a 1141-word function with heavy FP + many
calls; the C levers (pins, barriers, section attrs, prototypes, flags) either
change the access form (breaking the match) or do not move the register order.
Retain `INCLUDE_ASM`; full-ELF parity preserved.

## Raw-diff offset note

.text file offset = VMA - **0xFF480** (NOT -0xFF400). freeze.o .text data at
sh_offset 0x1200, DrawDialogText at +0x198. Compare `build/boot_elf.elf` vs
`assets/boot_elf.elf` from 0x1FBC50 for 0x11D4 bytes, or
`python3 working/draw_dialog_text/normdiff.py <N>` from the repo root.
