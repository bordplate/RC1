# pause_setLevelSpriteList (func_00221D28, code/game/pause_post2_levelsprite.cpp)

- 60 bytes (0x3C) at vram `0x221D28` (file `0x122CA8`).
  `int pause_setLevelSpriteList(PauseSpriteListMode* mode)`, bound to the
  address-based entry via `asm("func_00221D28")` symbol override (the
  generated function-pointer tables reference the address-based name).

## Semantics

Pause-menu callback in the original function-pointer table at `0x1D4578`
(referenced from `0x1D4020`/`0x1D4028`). It stages the current level's sprite
list into slot 1 (offset `0x34`) of the item's `spriteLists` and returns 0:

```cpp
mode->spriteLists[1] =
    pauseLevelSpriteLists[(u32)currentLevelId % PAUSE_LEVEL_SPRITE_LIST_COUNT];
```

- `currentLevelId` = `0x15ED84` (GP window): the active level id.
- `pauseLevelSpriteLists` = `0x1D4528`: a 19-slot per-level sprite-list
  pointer table, indexed by `currentLevelId % 19`; slot 18 is null. Entries
  point to 12-byte sprite records at `D_001D40C0..D_001D43E0`. Named in
  `config/symbols.txt`; the count is the `PAUSE_LEVEL_SPRITE_LIST_COUNT`
  (19) constant.
- `PauseSpriteListMode` (pause.h): `spriteLists[8]` at `+0x30`, so slot 1 is
  at `+0x34`. The sibling select handler `func_00221AB8` reads that same
  `0x34` field to drive the sprite list.

## Codegen

- The `(u32)` cast on `currentLevelId` is REQUIRED. The original emits
  `divu` (unsigned divide) followed by `mfhi` (the remainder) — i.e. an
  unsigned modulo. A signed expression would emit `div` and mismatch.
- The original loads `currentLevelId` as a single-register self-based
  absolute load (`lui $5, %hi; lw $5, %lo($5)`), NOT GP-relative, even though
  `0x15ED84` sits inside the gp window.
- `mode->spriteLists[1] = ...` lowers to the store `sw $2, 0x34($4)`; the
  table base `lui $6, %hi(pauseLevelSpriteLists)` and the `beql`/`divu`
  sequence reproduce the original byte-for-byte.

## Why the function has its own TU

This is a Splat boundary split (config/RC1.yaml, FILE offsets — the delta in
this region is 0x101080, NOT vram-0x100000):

- `game/pause_post2` [0x11DAB0, 0x122CA8) — head, `pause_post2.cpp`, `-G0`
- `game/pause_post2_levelsprite` [0x122CA8, 0x122CE8) — this function,
  `pause_post2_levelsprite.cpp`, default flags
- `game/pause_post3` [0x122CE8, 0x1286C0) — tail, `pause_post3.cpp`, `-G0`

The head and tail are pinned to `-G0` because `pause_resetMenuEntry`
(0x21DF30) needs EGC to inline the PI constant 0x40490FDB as a 3-instr
`lui; ori; mtc1` (under `-G8` EGC pools the float via `lwc1` from `.lit4`).
But under `-G0` the small-data threshold is below the 4-byte int, so the plain
`extern int currentLevelId` compiles to a two-register split load (base in a
second register, `li` scheduled between the `lui` and `lw`) instead of the
original's single self-based pair. Under the default `-G8` the plain scalar
extern is a bare small-data pseudo that ps2eeas expands in place to the
self-based `lui; lw` — exactly the original. This is the same mechanism as
`pause_updateSoundVolume` (see notes/pause_func_0021CB00.md).

Unlike that case (whose segment was exactly the function size), this segment
`[0x122CA8, 0x122CE8)` is 0x40 (64) bytes but the function is only 0x3C (60).
The extra 4 bytes (vram 0x221D64-0x221D67) are inter-function alignment
padding in the original (all `0x00000000`). They are reproduced automatically:
the `.text` section is aligned to 2**3, so the assembler pads the 60-byte
function up to the 64-byte section size with zeros — the object's `.text` is
exactly 0x40 and the next object lands on 0x221D68. No explicit padding source
is needed.

`pause_post3.cpp` re-declares the shared `pause_releaseSoundSlot`
(`asm("func_00225CD8")`) extern that the head's `pause_releaseStateSoundSlot`
also uses. Splat left 65 stale `.s` copies in the old `game/pause_post2`
matchings/nonmatchings directories; they were removed.

## Verification

- `tools/decomp_probe.py` with the production flags: 60/60 bytes, 0 diffs.
- `make clean && make split && make -j2`, then
  `cmp build/boot_elf.elf assets/boot_elf.elf`: byte-for-byte identical.
- `tools/decomp_status.py --count`: 573 -> 572.
