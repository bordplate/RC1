# pause_updateSoundVolume__Fv (func_0021CB00, vram 0x21CB00, file 0x11DA80, 48 bytes) - MATCHED 2026-09-08, REFACTORED 2026-09-16

## Behavior

Copies the raw music-volume option scaled by 8/10 into the pause sound
volume and returns zero:

```cpp
int pause_updateSoundVolume(void) {
    pauseSoundVolume = musicVolumeRaw * 8 / 10;
    return 0;
}
```

- `musicVolumeRaw` (0x15EDF0, GP window): the raw music-volume menu option,
  u32 in 0..1024 (ROM initial value 0x00000400 = 1024). `SoundOptionsMenu`
  (0x21CCA0/0x21CD04) steps it by +/-3 and clamps to [0, 1024]; the sibling
  raw option at 0x15EDEC is the sfx volume. `func_0022C8D0` (sound init) and
  `LoadingDataMenu` read it and derive the scaled values at 0x13E598..0x13E5AC.
- `pauseSoundVolume` (0x13E5A0, .data): the 80% music copy, equal to the
  0x13E598 value computed by the same x8/10 scaling.
- Ghidra has no function at 0x21CB00 and no code xrefs to it; it is a
  data-table leaf in the 0x1D1584 item table entry
  {SoundOptionsMenu, DrawSoundMenu, 0, func_0021CB00}.

## Matching form (2026-09-16)

The original load is a single-register self-based absolute load
(`lui v1, 0x16; lw v1, -0x1210(v1)`) — NOT GP-relative, even though
0x15EDF0 sits inside the gp window (gp = 0x166C00).

The matching C form is a plain scalar extern, no section attribute:

```cpp
extern int musicVolumeRaw;  // 0x15EDF0 in config/symbols.txt
extern int pauseSoundVolume __attribute__((section(".data")));
```

Mechanism (project flags `-G8 -O2 -ffast-math -fno-exceptions -snas`):
under the `-G8` small-data classification EGC treats the <=8-byte extern as
a local-ish object and emits ONE bare pseudo `lw $3, musicVolumeRaw` with
NO `.extern` declaration (EGC emits no `.extern` at all in C++ mode).
ps2eeas is single-pass: a `lw r, sym` reference with no preceding
`.extern sym,N` and outside a noreorder region expands in place to the
self-based absolute `lui r, %hi(sym); lw r, %lo(sym)(r)` — exactly the
original. The output store splits normally (hoisted `lui a1, 0x14` +
delay-slot `sw v1, off(a1)`). The `div`/`mflo` by the `li a0, 10` constant
reproduces the original division sequence byte-for-byte.

## Why the function has its own TU

`pause_post.o` is pinned to `-G0` because `pause_resetMenuEntry`
(0x21DF30) needs EGC to inline the PI constant 0x40490FDB as a 3-instr
`lui; ori; mtc1` (under `-G8`, EGC pools floats with nonzero lo16 via
`lwc1` from `.lit4`). But under `-G0` (and `-G2`) the small-data threshold
is below the 4-byte int, so the plain declaration compiles to the
2-register split form (base in `$v0`, `li a0, 10` scheduled between the
`lui` and `lw`, plus a move/sll swap — 5 word diffs, all probed).
`-G0 -mno-split-addresses` gives 6 diffs; inline-asm pseudo-load variants
give 8. So the function was isolated into its own default-`-G8` TU by a
Splat boundary split (config/RC1.yaml, file offsets — NOT vram-0x100000 in
this region, delta is 0x101080):

- `game/pause_post` [0x119F18, 0x11DA80) — head, `pause_post.cpp`, `-G0`
- `game/pause_post_soundvol` [0x11DA80, 0x11DAB0) — this function,
  `pause_post_soundvol.cpp`, default flags
- `game/pause_post2` [0x11DAB0, 0x1286C0) — tail, `pause_post2.cpp`, `-G0`

The shared `PauseSpriteListMode` typedef moved to `code/include/pause.h`
(used by head `pause_selectSpriteList` and tail `pause_setFirstSpriteTag`).
Splat leaves stale `.s` files in the old segment directories after a split;
they were removed (122 files) — the Makefile only uses them as
rebuild prerequisites, and stale copies confuse the generated layout.

## Verification

- `tools/decomp_probe.py` with the production flags: 48/48 bytes, 0 diffs
  (probe dir /tmp/opencode/pausevol/plain_g8b).
- `make clean && make split && make -j2`, then
  `cmp build/boot_elf.elf assets/boot_elf.elf`: byte-for-byte identical.
- `python3 tools/decomp_status.py --count`: 689 (unchanged; function stays
  matched in C).
- Object check: `build/code/game/pause_post_soundvol.o` contains
  `pause_updateSoundVolume__Fv` at exactly 0x30 bytes with the original
  structure (pre-link relocations zeroed).
