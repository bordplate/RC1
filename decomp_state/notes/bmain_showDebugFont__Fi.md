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
