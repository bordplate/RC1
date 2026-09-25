# Camera_updateCollMode (func_001ED940) — match notes

Address 0x1ED940, 0x120 bytes (72 words). Per-frame camera collision-mode/flag
update. Matched 2026-09-25 with default camera-TU flags (`-G8 -O2 -ffast-math
-fno-exceptions -snas`); full boot ELF parity passes.

## What it does
- `camCollFlag = 0x14`; set `0x34` when `(i208C-0x11) < 2 || i2084 == 0x73`.
- `camCollFlag = 0x14` when `i208C != 0x11 && f2F0 < currentCamera.posZ`.
- `camCollFlagPrev = camCollFlag; camCollFlag |= 0x80; camCollMode = 0xB4;`
- Mode chain on the level flags (mode is always `collMode | 0xB4`):
  - `c12E5` -> collMode 0x100
  - `c12EB` -> collMode 0xB00
  - `c12E6` -> collMode 0x300
  - `c12EC` -> collMode 0xD00
  - `c12E4` -> collMode 0
  - else -> camCollMode = camCollState.collMode | 0xB4 (collMode unchanged)

## Key codegen insight (the two things that took the search)
1. **Local pointer for base hoisting.** Taking `&camCollState` into a long-lived
   local (`CamCollState* q; q = &camCollState;` set first, used only in the mode
   chain) is what makes EGC hoist the camCollState base into the prologue (a0/a2)
   and allocate the levelCamData base to a1/a3. Accessing `camCollState` directly
   through the global materializes its base at point of use in the tail and loses
   the match (this was the wall across 5 natural forms).
2. **`beqzl` likely-branch is emergent.** The c12E4 test compiles to `beqzl`
   (opcode 0x14) — the only likely branch in the function — because, in the full
   if/else-if CFG, the else-only `q->collMode` load is scheduled into the ANNULLED
   delay slot of the beqzl (so when taken, v0 = c12E4 = 0 and the final
   `ori v0,v0,0xB4` yields 0xB4). Smaller standalone probes of `u8 != 0` / `> 0`
   in an else-if chain always emit plain `beqz`; the likely branch only appears
   with the complete dependent-load + return-tail structure.

## Related
- Verified struct fields added to `LevelCamInner` (f2F0 @+0x2F0, c12E4..c12EC
  @+0x12E4..0x12EC, i208C @+0x208C) and `CamCollState.collMode` @+0xC0.
- Globals camCollMode/camCollFlag/camCollFlagPrev (0x15EF98/9C/A0) were already in
  config/symbols.txt and link at the right addresses (parity confirms).
- Source shape mirrors `reference/Lombyte/src/textbin/fun_001ed940.c` (a known-
  matching reference). Last-resort-decompiler (GPT-5.6 Sol) located it after the
  primary agent exhausted 5 natural forms.
- Magic-number naming is tracked in refactor.json (camera_Camera_updateCollMode).
