# ResetVideoPipeline__Fv (0x1f21c0, 156 bytes)

VIF1/DMAC/GS/VU1 video-pipeline reset, used as VU1_syncChain's sit-and-spin
timeout recovery. The texture cursor end is saved across SetPalMode, which would
otherwise rewind it to the texture memory base.

```cpp
spaceLoadInProgress = 1;          // 0x15F618
vifChainFlags = 0;                // 0x160EE0
DMAC_VIF1_Disable();
sceDmaReset(1);
InitDma();
sceGsResetGraph(0, 1, videoModePal ? GS_VIDEO_PAL : GS_VIDEO_NTSC, GS_FIELD_MODE);
resetVif1Gif();
VU1_initChain();
int saved = textureCursorEnd;     // 0x15EE78
vu1ChainHead = 0;                 // 0x160F00
SetPalMode();
textureCursorEnd = saved;
VU1_initChain();
DMAC_VIF1_Enable();
```

## Two codegen constraints

### 1. Standalone absolute accesses (the 16-byte gap)

The original keeps FOUR accesses as standalone self-based absolute pairs
(two instructions): `spaceLoadInProgress=1`, `vifChainFlags=0`, the
`videoModePal` load, and the `textureCursorEnd` load. The `videoModePal` load is
naturally standalone (its value feeds the `movz`). The other three are
independent, so EGC's scheduler normally moves them into the preceding jal's
delay slot, where ps2eeas expands the bare pseudo to a single GPREL16 (one
instruction) instead of the absolute pair — 16 bytes short.

ps2eeas (single-pass) expands a bare pseudo `sw/lw r,sym`:
- to a single GPREL16 when the pseudo is inside a `.set noreorder` region (a
  jal delay slot) or preceded by `.extern sym,N`;
- to an absolute self-based pair (`lui r; sw/lw r,off(r)` / `lui at; sw`)
  otherwise.

So the addressing mode is determined by whether the scheduler places the access
in a delay slot. A full memory barrier (`asm volatile("" : : : "memory")`)
after the access prevents the delay-slot placement and forces the absolute
pair — but the barrier also creates a scheduling boundary that (a) moves the
prologue `sq ra;sq s0` to after the two stores, and (b) shifts the two trailing
GP stores into the next call's delay slot. That left 8 word diffs.

The working form (expert-recommended): a `.data`-classified alias at the same
address plus `-mno-split-addresses` on the TU. This makes EGC lower the access
to an unsplittable self-based absolute pseudo that ps2eeas expands in place to
the two-instruction pair, with NO scheduling boundary. Aliases in
`config/linker_aliases.ld`:

```ld
resetVideoVifChainFlagsAbs = 0x00160EE0;   // = vifChainFlags
resetVideoTextureCursorEndAbs = 0x0015EE78; // = textureCursorEnd (load only)
```

Declared in source as `extern int resetVideoVifChainFlagsAbs
__attribute__((section(".data")));` (and the same for the textureCursorEnd
alias). The `textureCursorEnd` store still uses the seeded GP alias
`textureCursorEndGp` (`.extern textureCursorEndGp, 4`), mirroring
camera.cpp's screenFade/screenFadeGp mixed-mode pair.

### 2. Trailing GP store delay-slot placement

The two trailing GP stores (`vu1ChainHead=0` in SetPalMode's delay slot,
`textureCursorEnd=saved` in the second VU1_initChain's delay slot,
DMAC_VIF1_Enable's slot = nop) sit in the delay slot of the call the source
line FOLLOWS. This EGC schedules a store into the delay slot of the call it
PRECEDES, so the source writes each store BEFORE the call whose delay slot
holds it:

```cpp
int saved = resetVideoTextureCursorEndAbs;
vu1ChainHeadStore = 0;   // before SetPalMode -> SetPalMode's delay slot
SetPalMode();
textureCursorEndGp = saved; // before 2nd VU1_initChain -> its delay slot
VU1_initChain();
DMAC_VIF1_Enable();
```

Both reorderings are semantically safe: SetPalMode rewrites
textureCursor/textureCursorEnd (not vu1ChainHead) and VU1_initChain does not
read textureCursorEnd.

## TU isolation (Splat segment split)

`-mno-split-addresses` breaks the sibling draw_post functions (a TU-wide build
differed in 67 functions), so ResetVideoPipeline was split into its own Splat
segment/TU. `config/RC1.yaml`:

```yaml
- [0xf2ff0, cpp, game/draw_post]          # projectWorldPoint + dead tail + noops
- [0xf3140, cpp, game/draw_post_reset]    # ResetVideoPipeline (this note)
- [0xf31e0, cpp, game/draw_post_post]     # func_001F2260 .. func_001F7A88
```

`code/game/draw_post_reset.cpp` holds ResetVideoPipeline; the Makefile gives
`draw_post_reset.o` `PRIVATE_COMPILE_FLAGS = -mno-split-addresses`. The other
two TUs keep the default flags.

## Verification

`tools/decomp_probe.py` matched byte-for-byte (0 diffs, 156 bytes); a clean
`make clean && make split && make -j2` + `cmp build/boot_elf.elf
assets/boot_elf.elf` passes.

## Variants that did NOT work (for future reference)

- Full memory barriers after the two accesses: correct size/addressing but 8
  word diffs (prologue reorder + tail-store shift).
- Tied barrier on the loaded value (`asm volatile("" : "+r"(saved))`): same 8 diffs.
- `-fno-schedule-insns` / `-fno-schedule-insns2` alone or with barriers: worse
  (140 bytes or reorders the sceGsResetGraph argument loads).
- `.data` on the original symbols without `-mno-split-addresses`: wrong split
  form (148 bytes, 34 diffs).
- Tail barriers between store and following call: 168 bytes (over-scheduled).
