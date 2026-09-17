# sound_GetFade__FP9SoundDataP4vec4 (0x22C7E8, 0x44) — MATCHED 2026-09-17

Volume of a 3D sound channel at its distance from the listener:
`dist = FastVecDist(pos, Camera)`, then
`return sound_GetFade(channel->def, dist, def->minRange, def->maxRange)`.

## Key reading

The second call's first float arg is NOT a constant: `mov.s f12, f0` moves
FastVecDist's return value (f0) into the argument slot. The callee reads
`f12` = dist, `f13` = `lwc1 0(a0)` = def->minRange, `f14` = `lwc1 4(a0)` =
def->maxRange. (An earlier draft read this as 0.0f; the VU distance call is
the reason it exists — the result feeds the fade, it is not discarded.)

## Naming evidence (Deadlocked reference)

Deadlocked's sound module (reference/dl/FUNCTIONS.txt, file path
`X:\rcb\code\stable\game\sound.cpp`) contains the direct successors:

- `sound_GetFade__FP8SoundDeffffb` (SoundDef*, dist, minRange, maxRange,
  bForceExp) — same attenuation algorithm (linear vs squared by `flags & 1`,
  maxVolume at dist<=minRange, minVolume at dist>=maxRange). RC1's
  0x22C6F8 is the 4-arg ancestor (no bForceExp, no Level check). Renamed in
  config/symbols.txt; still INCLUDE_ASM.
- `sound_GetFade__FP11_sound_dataR4vec4P4vec4b` (SoundData* channel, vec4*
  soundPos, vec4* camPos, bForceExp) — RC1's 0x22C7E8 is the 2-arg ancestor
  with the camera position hardcoded to the global `Camera` (0x187080)
  instead of a parameter. Insomniac overloaded `sound_GetFade` in both eras.

## Verified data layout

Slot array: 0x13E5C0 + i*0x70 (free-slot scan reads status at
0x13E5C4 + i*0x70; up to 0x1a or 0x1e slots by level type). Verified against
raw objdump of the channel start function 0x22D7F0 (base register = slot
base - 0x70, i.e. all store offsets below are slot-relative):

```
SoundData (0x70 bytes):
  +0x00 handle      (int, -1 on commit; DL: channel.handle)
  +0x04 status      (char, 0 free / 7 active; DL: channel.status = '\a')
  +0x05 flags       (char, start param_2; 0x10 = skip GetFade call)
  +0x08 def         (SoundDef*)
  +0x0C index       (u16, = def->index)
  +0x0E def_index   (u16, = 0xffff; DL: channel.def_index)
  +0x10 volumeMod   (int, start param_5; DL: channel.volumeMod)
  +0x14 pitch       (int, minPitch + rand(maxPitch-minPitch); DL: channel.pitch)
  +0x18 pMoby       (MobyInstance*, 0 at start; pos read at pMoby->pos +0x10)
  +0x1C field_0x1C  (int, 0 at start)
  +0x20 pos         (vec4, start param_4 / moby pos / zeroed)
  +0x30 offset      (vec4, zeroed at start; DL: channel.offset)
  +0x40 field_0x40  (int, zeroed on commit)
```

```
SoundDef:
  +0x00 minRange (f32)   +0x04 maxRange (f32)
  +0x08 minVolume (int)  +0x0C maxVolume (int)
  +0x10 minPitch (int)   +0x14 maxPitch (int)
  +0x18 loop (u8)        +0x19 flags (u8, bit 0 squared fade)
  +0x1A index (u16)      +0x1C field_0x1C (u32)
```

Callers: sound_update (0x22CA50) at 0x22D004 — result feeds
`min(volumeMod * fade, 1023) >> 10`; start function 0x22D7F0 at 0x22D9BC —
result gates the commit (`< 0x20` → fail).

## Codegen

No special flags; default TU flags. EGC reproduced the hoisted
`lui v0, %hi(Camera)`, `move a0, a1`, the reload `lw a0, 8(s0)` after the
first call, and the `lwc1 f14, 4(a0)` in the second call's delay slot
exactly. `Camera` (0x187080, .data, out of the GP window) is declared
`extern vec4` — no address casts.

## vec4 fix (part of this change)

`struct vec4 { s128 ... }` was 8 bytes: this EGC has no 16-byte scalar
(`long long` = 8, verified by layout probe), so the old spelling was wrong.
Deadlocked's symbols show the original class held one 128-bit member; the
fix spells the components out: `struct vec4 { f32 x, y, z, w; }` (16 bytes).
This restores MobyInstance's documented layout (field26_0x50 ... field84_0xbf
now align; sizeof = 0x100 = the original `addiu s0,s0,0x100` stride in
CreateMoby) and makes SoundData's 0x70 size honest. The misleading
`s128`/`u128` typedefs (both 8-byte `long long`) were removed from types.h;
vec4 was their only user. EGC emits 16-byte struct copies as
ldl/ldr/sdl/sdr pairs (probe-verified) — the genuine output of this compiler.
mobyfunc.cpp's experimental CreateMoby candidate (SKIP_ASM only) had
`moby += 0x100`, which is 0x100 elements; corrected to `moby += 1` now that
sizeof(MobyInstance) == 0x100.
