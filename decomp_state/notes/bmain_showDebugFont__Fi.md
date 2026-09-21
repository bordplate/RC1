# showDebugFont__Fi (code/game/bmain.cpp) — MATCHED 2026-09-21

`void showDebugFont(int index)` at vram `0x001E9488`, 0x1D0 (464) bytes, 116
instructions. C++-mangled `showDebugFont__Fi` (1 int arg, void return). Sibling
`startlevel__Fv` (0x1E9658, 0x45C) stays `INCLUDE_ASM` (out of scope). bmain.o
uses the default flags `-G8 -O2 -ffast-math -fno-exceptions -snas` (no private
flags).

## Identity / context

Plays boot stream table entry `index` — the debug-font movie. No-op for a
negative index. Sequence: stop audio, fade out, set `decodeMode=2` and
`GameMode=1`, wait for the menu to settle, run the movie decode on the level
decode buffer, refresh the HUD, then reset `decodeMode=0` / `GameMode=0` and
fade back in.

```c
void showDebugFont(int index) {
    if (index < 0) return;
    // font {src,size} from debugFontLoadInfo (see below), NTSC vs non-NTSC table
    decodeMode = 2;
    audioState.playbackFlags |= 0x8;
    FlushCache(0); sound_StopAllSounds(); music_Stop();
    FadeToBlack(func_001F96F8(0xC));
    GameMode = 1;
    FlushCache(0); sound_StopAllSounds(); music_Stop();
    snd_StreamSafeCdSync(0);
    while (menuStateData.field_0xD4 >= 3 || menuStateData.field_0xDC >= 0)
        memcard_Update();
    u32 decBase = levelMem.decodeBufBase;
    func_0023A3B8(fontSrc, fontSize, decBase + 0x100000, decBase + 0x400000, 0);
    func_00120C30(0); func_00122298(0); func_00120558(0, 0);
    func_00122E68(vsync_callback);
    Hud_sendTexture((char*)0x1000000, frameBufferBase, 0x1B, 6, 6, 1);
    decodeMode = 0; FadeToBlack(4); GameMode = 0;
    audioState.playbackFlags |= 0x10;
}
```

Caller: `Transition_DoTransition__Fv` (0x1EBB34, `jal showDebugFont__Fi`).

## Globals

- `debugFontLoadInfo` (0x137B80, out of GP window, `.data`): the boot font
  table. 16-byte head is the debug font image (`LoadDebugFont`, bloaders.cpp);
  the 8-byte `{u32 src; u32 size;}` records much further in describe the
  streams this function plays. NTSC table at +0x1A98 (src) / +0x1A9C (size),
  non-NTSC at +0x1A78 / +0x1A7C (0x20 apart). Declared as `DebugFontLoadInfo`
  with `__attribute__((section(".data")))`.
- `audioState` (0x13E550, out of window, `.data`): the byte at +0x6B (0x13E5BB)
  is a playback flag (bit 3 = starting, bit 4 = finished). `pauseSoundVolume`
  (0x13E5A0) is +0x50 of the same block.
- `levelMem` (0x1940C0, out of window, `.data`): level memory map; `decodeBufBase`
  is the 0x1C field. `MemSlots` (0x1940C4) and `hudHeapBase` (0x1940CC) are the
  0x04/0x0C fields of this same block.
- In-window scalars (plain externs, keep the `-G8` bare pseudo):
  `NTSCProgressive` (0x15ED80), `decodeMode` (0x15EED8), `frameBufferBase`
  (0x15EE88), `GameMode` (0x15F604).

## Callee signatures

`FadeToBlack` is defined mangled `FadeToBlack__FiUi` (int, unsigned int) but no
boot caller materializes `a1`; declared one-arg with an `asm("FadeToBlack__FiUi")`
pin so the call sets only `a0`. `func_0023A3B8(src, size, video, audio, flags)`
is the movie/audio decode setup (same family as the movie readMpeg path).
`0x1000000` is the EE VRAM/GS base (hardware region), not a data symbol.

## Codegen: font-record address (the only non-natural region)

The original computes the font-record address `base + index*8` and keeps it in
TWO registers:

```
addu  a0,v0,v1     ; a0 = base + index*8   (32-bit)
move  v0,a0         ; v0 = base + index*8   (64-bit zero-extend)
lw    s1,0x1A9C(a0) ; fontSize from a0, MAIN LINE
b     ...
lw    s2,0x1A98(v0) ; fontSrc  from v0,  DELAY SLOT   (non-NTSC: no branch)
```

i.e. `s1=fontSize`, `s2=fontSrc`, fontSize loaded first. The base hi (`lui v0,
0x13`) is CSE'd into the `beqz` delay slot and shared by both branches.

This EGC build (2.95.2 SN 2.73a) cannot reproduce that from natural pointer
arithmetic:
- Through a `u8* p` variable it keeps the address in ONE register (`v1`) for
  both loads (no 64-bit `move`), 2 instructions short (114 vs 116).
- The C statement order simultaneously controls which load lands in the `b`
  delay slot AND the s1/s2 assignment, and the coupling is the OPPOSITE of the
  original's: my EGC maps delay-load→s1 / main-load→s2, the original maps
  main-load→s1 / delay-load→s2. So the original's combination
  (fontSize main/s1, fontSrc delay/s2) is unreachable: putting fontSize in the
  main line forces it to s2 here.
- Inline pointer (`*(u32*)((u8*)&dbg + index*8 + off)`) folds the offsets into
  the address (lw offsets become 0/-4) — mangled. A `u32` pointer cast fails to
  compile (EGC already uses 32-bit pointers). `-fno-schedule-insns` fixes the
  load order but re-allocates the values to s3/s4 and breaks the rest.

Fix (last-resort-verified): pin the values to fixed registers and emit the two
address instructions with inline asm, so EGC reuses the exact registers the
original used:

```cpp
register u32 fontSize asm("$17");   // s1
register u32 fontSrc  asm("$18");   // s2
register u32 scaled asm("$3") = index * 8;      // v1 = index*8
register u8* base   asm("$2") = (u8*)&debugFontLoadInfo; // v0 = base
register u8* p      asm("$4");                       // a0
asm volatile("addu %0,%1,%2" : "=r"(p) : "r"(base), "r"(scaled)); // addu a0,v0,v1
register u8* q      asm("$2");                       // v0
asm volatile("daddu %0,%1,$0" : "=r"(q) : "r"(p));   // move v0,a0
fontSize = *(u32*)(p + 0x1A9C);   // lw s1,0x1A9C(a0)
fontSrc  = *(u32*)(q + 0x1A98);   // lw s2,0x1A98(v0)
```

This reproduces the exact bytes for both branches (objdump + full linked-ELF
cmp verified).

## GPREL misread correction

The mid-function store was initially modeled as `D_0015F624 = 1;` (a `.lit`
placeholder). The function's ONLY gp-relative store is `sw v0, -30204(gp)`
= `0x166C00 - 0x75FC` = **0x15F604 = `GameMode`**, so the correct statement is
`GameMode = 1;` (later reset by `GameMode = 0;`). The other in-window stores
(`decodeMode = 2/0`) use an absolute `lui at / sw` base, not GPREL. A
structural diff that strips immediates misses this (both are `sw v0,<off>(gp)`);
only a byte comparison catches the 0x15F604 vs 0x15F624 difference. This is the
Ghidra Gp-name pitfall from AGENTS.md.

## Verification

bmain.o compiled with default flags; `showDebugFont__Fi` is exactly 0x1D0 bytes
(`startlevel__Fv` lands at 0x1E9658). Function region 0x1E9488–0x1E9658
byte-identical to `assets/boot_elf.elf`; full `cmp build/boot_elf.elf
assets/boot_elf.elf` passes. `func_001E9488` is no longer referenced by any
`INCLUDE_ASM` and is absent from the built object (the leftover generated
`func_001E9488.s` is not compiled).

## Refactor: named constants (2026-09-21, refactor.json entry cleared)

Pure textual substitution of the magic numbers with `#define`s (verified
byte-identical: function region 0x1E9488–0x1E9658 and full ELF cmp pass
afterwards). Constants added in bmain.cpp:

- Font record offsets: `DEBUG_FONT_NTSC_SRC/SIZE` (0x1A98/0x1A9C),
  `DEBUG_FONT_NON_NTSC_SRC/SIZE` (0x1A78/0x1A7C) — offsets from
  `debugFontLoadInfo`.
- Decode sub-buffer offsets: `LEVEL_DECODE_VIDEO_OFFSET` (0x100000),
  `LEVEL_DECODE_AUDIO_OFFSET` (0x400000) — within `levelMem.decodeBufBase`.
  The audio one matches the movie path: the decode setup stores it in the
  global the decode loop advances by `MOVIE_VIBUF_OFFSET`/
  `MOVIE_AUDIO_DEC_OFFSET` (audiodec.h) for the VIF/VAG buffers.
- Texture transfer params: `DEBUG_FONT_TEX_FORMAT` (0x1B),
  `DEBUG_FONT_TEX_U_LOG` (6), `DEBUG_FONT_TEX_V_LOG` (6),
  `DEBUG_FONT_TEX_MODE_IMMEDIATE` (1). The entry's "color parameters"
  label was wrong: Ghidra on `Hud_sendTexture__FPciiiii` (0x200B10) shows
  the 3rd arg passed to the VU0 program as the GS texture format
  (0x1B = PSMCT16SH4, 16-bit color + 4-bit shared alpha), the 4th/5th as
  log2 width/height (used as `1 << n`, 64x64 here), and the 6th as a mode:
  0 appends the transfer to the current VU1 chain, 1 does
  `FlushCache(0)` + `func_00122658(packet, dest)` immediately.
- Fade durations: `DEBUG_FONT_FADE_OUT_FRAMES` (0xC, through the
  `func_001F96F8` scale), `DEBUG_FONT_FADE_IN_FRAMES` (4, direct).
  `FadeToBlack`'s argument is the GS alpha step count (one loop iteration
  per frame); `FadeToBlack(4)` also appears in the level loop
  (FUN_001EB798) as the quick fade.
- Playback flag bits: `AUDIO_PLAYBACK_FLAG_STARTING` (0x8),
  `AUDIO_PLAYBACK_FLAG_FINISHED` (0x10).
- Mode values: `DECODE_MODE_NORMAL` (0) / `DECODE_MODE_DEBUG_FONT` (2),
  `GAME_MODE_NORMAL` (0) / `GAME_MODE_DEBUG_FONT` (1).

Research backing the names:

- `func_001F96F8` (fastfunc.s) is a frame-rate scaler:
  `(int)(n * *(float*)0x15ED68 + 0.2f)`. The slot is 1.0f in the boot ELF
  (0x15ED60/64/68 are all 1.0f; 0x15ED6C–0x15ED7C hold 1/60, 1/3600,
  1/216000 frame-time constants), so it is the identity here. No boot-ELF
  writer to those slots found (an overlay may rescale them).
- `decodeMode` (0x15EED8) value set: startlevel writes -1 at 0x1E9844; the
  movie decode loop FUN_0023A460 gates on `!= -1` / `!= 2` / `== 0`.
- `GameMode` (0x15F604): the level loop FUN_001EB798 skips normal updates
  while it is nonzero; 3 is the pause value (pause_scheduleInput; also
  stored at 0x218F84). Many other writers pass the value as a parameter.
- The playbackFlags byte 0x13E5BB is shared with the space clone as the
  note's identity section said: the space sound-update code at 0x22CA90
  reads it (`andi` 0x8, 0x13, 0x4 around the byte plus neighbor fields at
  +0x64/0x68–0x6A of audioState), and the space debug-font player at
  0x231640+ sets 0x8, calls the same `func_0023A3B8` setup and
  `FadeToBlack(4)`. Its font records sit at +0x1938/+0x193C of its own
  table copy (0x137B80 + index*8), not at the boot table's 0x1A78/0x1A98.

Style follow-up recorded: the remaining `func_XXXX` callee prototypes in
bmain.cpp still lack the per-declaration C-linkage evidence / unknown-symbol
comments STYLEGUIDE.md requires (new refactor.json entry
`bmain_callee_prototype_comments`).
