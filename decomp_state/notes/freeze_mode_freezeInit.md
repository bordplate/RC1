# mode_freezeInit (0x1FBAB8) — blocked on switch-jtbl placement

Function: `code/game/freeze.cpp` : `mode_freezeInit` (VRAM 0x1FBAB8, size 0x198, C-style
symbol `mode_freezeInit`). Sets up the pause/freeze dialog state.

## Semantics (verified against original asm + delay-slot semantics)
```
void mode_freezeInit(int dialog, int restorePage):   // dialog is compared UNSIGNED (sltiu)
  if (GameMode != 3) { snd_PauseAllSoundsInGroup(0x1D); music_Pause(0); }
  freeze.f14 = GameMode;             // GameMode=0x15F604 (3=pause,4=freeze); freeze=struct @0x193300
  freeze.f18 = restorePage;
  GameMode = 4;
  freeze.f00 = dialog;               // stored in the bounds-check (beqz) DELAY SLOT
  switch (dialog) {                  // DENSE 7-case (0..6); single `sltiu` is the guard, not an outer if
    case 0: f08=msg_string(0x4F6E); f0C=msg_string(0x5248); f10=msg_string(0x5249);
            f28=0; f04=0; f1C=0; f20=0; f24=0; break;
    case 1: case 4: f08=msg_string(0x5229); f0C=msg_string(0x4EE0); f10=msg_string(0x524A); f04=0; break;
    case 2: f08=msg_string(0x524A); f04=0; f0C=0; break;
    case 5: func_001FED30(0x4E2B); goto shared;
    case 3: shared: f04=func_001F96F8(0x1E); f20=0; f24=func_001F96F8(0x1E); break;  // TWO calls, f20=0
    case 6: f04=func_001F96F8(0x1E); f1C=0; break;
    default: f08=0; f04=0x78; f0C=0; f10=0; break;
  }
```
CORRECTION (last-resort GPT-5.6 Sol): cases 3/5 make TWO `func_001F96F8(30)` calls, not three —
the store at 0x1FBC00 is `sw zero,0x20(s0)` in the second call's delay slot, so f20=0 (not a call).
Also: do NOT write `if ((unsigned)dialog < 7)` — the single `sltiu v1,s0,7` at 0x1FBB14 is the
bounds check EGC emits for a plain dense `switch`+`default`; an explicit outer if changes codegen.

Callees (named): snd_PauseAllSoundsInGroup=0x12e3e8, msg_string__Fi=0x1fdd10, music_Pause__Fi=0x216050.
Unnamed (Splat placeholders, real names TBD): func_001F96F8 (frames*float*2), func_001FED30.
freeze struct @0x193300: +0x00 dialog, +0x04 timer, +0x08 title, +0x0C optionA, +0x10 optionB,
+0x14 previousGameMode, +0x18 restorePage, +0x1C f1C, +0x20 f20, +0x24 f24, +0x28 f28.

## THE BLOCKER — dense-switch jumptable cannot be placed in the raw data blob
Original dispatch (objdump ground truth):
```
1fbb28: lui   v0,0x1e          # %hi(jtbl_001E78D0)
1fbb2c: sll   v1,s0,2          # s0 = dialog
1fbb30: addiu v0,v0,30928      # %lo(jtbl)
1fbb34: addu  v1,v1,v0
1fbb38: lw    a0,0(v1)
1fbb3c: jr    a0
1fbb40: nop
```
jtbl_001E78D0: 0x20 bytes (7 entries + 1 pad), ALL instruction-aligned:
`1FBB44 1FBBA0 1FBB84 1FBBE4 1FBBA0 1FBBD0 1FBC0C 00000000` (case0..6, pad).
It sits INSIDE the `data` segment (0x165480-0x1E8B80) which Splat models as ONE raw blob
(`data.data.o`, assembled from `code/_generated/build/data/data.data.s`).

A standalone EGC probe of this exact 7-case switch emits the jtbl into the OBJECT'S `.rodata`
(0x20 bytes) with the lui/sll/addiu/addu/lw/jr dispatch. (A sparse switch over cases 0,1,2,4
emits a branch cascade instead — wrong codegen; the switch must stay dense 0..6.)

Splat 0.50.0 (`ld_legacy_generation: True`) places the whole data segment as the raw blob and
routes ALL compiled objects' `.data`/`.rodata` to a "text_DATA" region AFTER `.text`
(~0x23d360). There is NO per-object data placement at original addresses. So compiling in C puts
the jtbl after .text and the dispatch references it there — not 0x1E78D0. The raw blob still
holds the correct jtbl bytes at 0x1E78D0 (dead). Result: 0x20 extra bytes after .text + wrong
dispatch target => `cmp build/boot_elf.elf assets/boot_elf.elf` FAILS.

## Ruled out (all verified)
1. Data-segment `rodata` subsegment in config/RC1.yaml at ROM 0xE8850 (split data into
   [data 0x66400][rodata 0xE8850][data 0xE8870]): Splat emitted a RAW blob for the rodata
   subsegment but the .ld STILL placed the whole data.data.o and STILL routed object .rodata
   after .text. Data subsegments do NOT create per-object linker entries here. It also produced
   duplicate data .s files (data.data.s + 66400.data.s/E8850.rodata.s/E8870.data.s) => dup-symbol
   risk. REVERTED; baseline parity re-confirmed OK.
2. `pair_rodata_to_text`: config/RC1.yaml comment "Not having this crashes splat" => unusable.
3. Inline-asm dispatch / labels-as-values computed-goto / table `section` attribute / linker
   alias / `FILL` / overlapping output sections: do not create the needed "hole" in the blob or
   re-target the compiler's local section-symbol relocation (expert GPT-6 Astra assessment).

## Last-resort GPT-5.6 Sol recommendation (tested -> flawed as given)
Last-resort prescribed an `.incbin` splice: a `data_freeze_splice.o` with
`.section .data.freeze_before` = `.incbin "assets/boot_elf.elf", 0x66400, 0x82450` and
`.section .data.freeze_after` = `.incbin "assets/boot_elf.elf", 0xE8870, 0x1290`, splicing
`freeze.o(.rodata)` between them inside the `.data` output section (prefix 0x82450 + jtbl 0x20 +
suffix 0x1290 = 0x83700 = data segment size).

TESTED: assembled that `.incbin` object and ran `nm` on it => it defines ZERO symbols. The data
segment is full of REFERENCED globals defined via `dlabel` in data.data.s (e.g. vu1ChainTail,
levelBssStart, the freeze struct, hundreds of D_XXXXXX). Replacing data.data.o with a symbol-less
`.incbin` object leaves every referenced data global in 0x165480-0x1E8B80 UNDEFINED => link fails.
So the `.incbin` splice as prescribed does NOT work.

## The real fix (deferred tooling project — NOT a focused iteration)
A SYMBOL-PRESERVING split is required: split data.data.s into data_prefix.s (0x165480-0x1E78D0,
keeping its dlabels) and data_suffix.s (0x1E78F0-0x1E8B80, keeping its dlabels), build them to
objects, and patch the generated .ld (post-`make split`, so it isn't overwritten) to place
`data_prefix.o(.data)` + `freeze.o(.rodata)` + `data_suffix.o(.data)` at 0x165480 with
`ASSERT(. == 0x1E78D0)` before and `ASSERT(. == 0x1E78F0)` after the `.rodata`. Also update the
Makefile to drop data.data.o and add the two split objects. Then the jtbl bytes at 0x1E78D0 come
from freeze.o's .rodata (whose entries are relocations to the case labels).

INDEPENDENTLY of placement, the CODEGEN must also match: the candidate's case bodies must land at
the EXACT original offsets (0x1FBB44, 0x1FBBAA, 0x1FBB84, 0x1FBBE4, 0x1FBBD0, 0x1FBC0C) so the
jtbl relocations resolve to the original bytes; plus the delay-slot store ordering (case-0
five-store tail, case-2 reversed store, case-3/5 two-call+f20=0, default four-store rotation) and
the `freeze.f00 = dialog` store hoisted into the bounds-check delay slot.

## Scope note
This blocks mode_freezeInit AND would affect ANY function whose dense switch makes EGC emit a
.rodata jtbl that the original placed inside the raw data segment. A general solution (the
symbol-preserving data split) is a dedicated build/tooling improvement, to be committed SEPARATELY
from any function decompilation (per AGENTS.md tooling rule).

## Current state
Function retained as INCLUDE_ASM; full-ELF parity preserved (`make split && make && cmp` => OK).
Baseline verified 2026-10-04.
