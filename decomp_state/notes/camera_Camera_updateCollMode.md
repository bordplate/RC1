# Camera_updateCollMode (func_001ED940) — match notes

Address 0x1ED940, 0x120 bytes (72 words). Per-frame camera collision-mode/flag
update. Matched 2026-09-25 with default camera-TU flags (`-G8 -O2 -ffast-math
-fno-exceptions -snas`); full boot ELF parity passes.

## What it does
- `camCollFlag = 0x14`; set `0x34` when `(heroStateType-0x11) < 2 || heroState == 0x73`.
- `camCollFlag = 0x14` when `heroStateType != 0x11 && waterHeight < currentCamera.posZ`.
- `camCollFlagPrev = camCollFlag; camCollFlag |= 0x80; camCollMode = 0xB4;`
- Mode chain on the level's hotspot flags (mode is always `collMode | 0xB4`):
  - `hotSpotLava` -> collMode 0x100
  - `hotSpotDeathSand` -> collMode 0xB00
  - `hotSpotQuickSand` -> collMode 0x300
  - `hotSpotIceWater` -> collMode 0xD00
  - `hotSpotWater` -> collMode 0
  - else -> camCollMode = camCollState.collMode | 0xB4 (collMode unchanged)

## Confirmed semantics (2026-09-25 refactor)
Deadlocked's `Cam_HandleHotspots` (reference/dl/game_dl/camera.cpp) is the
direct descendant of this function, with the same flag values plus a 0x1000
bit (0x1004/0x1024/0x10A4):
- 0x14/0x34 are the sphere-collision flags; 0x34 (bit 0x20) is selected while
  the hero is in a WATER state: stateType 0x11/0x12 (Deadlocked's
  HERO_TYPE_SWIM/SURF occupy exactly those values) or state 0x73 (Deadlocked
  checks 0x72 = HERO_STATE_WADE; RC1's hero state enum is numbered one
  lower/higher, so 0x73 is the RC1 wade-equivalent, not literally enum slot
  115 of the DL table).
- 0x80 is the active bit, OR'd in after the pre-active value is saved to
  camCollFlagPrev (DL: CAMERA_COLL_SPHERE_FLAGS = v | 0x80,
  CAMERA_COLL_BUMP_FLAGS = v).
- 0xB4 is the base bump/hotspot mode (DL: CAMERA_COLL_FLAGS = 0x10A4); each
  hotspot mode is OR'd with it (DL: CAMERA_COLL_FLAGS = hotspot | 0x10A4).
- Hotspot values 0x100/0xB00/0x300/0xD00/0 map 1:1 to DL's
  camHeroData.hotSpotLava/DeathSand/QuickSand/IceWater/Water fields, in the
  same priority order.
- The three globals (camCollFlag/camCollFlagPrev/camCollMode) are write-only
  in the boot ELF; level overlays consume them.
All literals are now named constants (CAM_COLL_FLAG_*, CAM_COLL_MODE_BASE,
CAM_HOTSPOT_*, CAM_HERO_STATE_*) and the LevelCamInner fields renamed
(hotSpot*, heroState/heroStateType, waterHeight); the refactor kept
camera.o byte-identical (26/26 function diff + full boot parity).

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
- Verified struct fields in `LevelCamInner` (waterHeight @+0x2F0,
  hotSpotWater/Lava/QuickSand/DeathSand/IceWater @+0x12E4..0x12EC,
  heroState @+0x2084, heroStateType @+0x208C) and `CamCollState.collMode` @+0xC0.
- Globals camCollMode/camCollFlag/camCollFlagPrev (0x15EF98/9C/A0) were already in
  config/symbols.txt and link at the right addresses (parity confirms).
- Source shape mirrors `reference/Lombyte/src/textbin/fun_001ed940.c` (a known-
  matching reference). Last-resort-decompiler (GPT-5.6 Sol) located it after the
  primary agent exhausted 5 natural forms.
- Magic-number naming (refactor.json camera_Camera_updateCollMode) was applied
  2026-09-25 and verified; the entry is cleared.
