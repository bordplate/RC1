# FadeToBlack__FiUi (0x1F4A58, 0x188 = 98 words) — MATCHED

File: `code/game/draw_post_post.cpp`. Matched 2026-09-29. First-try probe
matched byte-for-byte with the default TU flags (`-G8 -O2 -ffast-math
-fno-exceptions`, SN assembler); clean `make clean && make split && make -j2`
+ `cmp build/boot_elf.elf assets/boot_elf.elf` pass.

## Semantics
Fades the display to black over `frames` frames. Preamble:
`VU1_syncChain(1); func_00122298(0); drawFrameCount++; VU1_initChain();`.
Then, for `i = frames-1 .. 0`:
- `PutDrawBufferLarge(); framebuf_appendLargeSetup();`
- `func_001F5210(0,0,0,0x80)` — still-assembly helper that writes GS register
  1 to full (0x80000000) and streams the 80-word GS state block at 0x13CC90.
- `PutDrawBufferSmall();`
- `VU1_addGSregister(VU1_FADE_GS_REG=1, (0x80 - (i<<7)/(i+1)) << 24)` — steps
  the fade value: top byte goes from ~0 (visible) to 0x80 (fully black) as i
  counts down.
- streams the `gsStateFade` block (0x13CDD0) as a VIF data-reference packet
  `[0x30000014, (u32)gsStateFade, 0, 0x50000014]` and advances
  `vu1ChainHead += 4`.
- `VU1_syncChain(1); func_00122298(0); drawFrameCount++; VU1_sendChain();
  VU1_swapChain();`
Post-loop: `VU1_syncChain(1); func_00122298(0); drawFrameCount++;
VU1_initChain(); PutDrawBufferLarge(); framebuf_appendLargeSetup();` (runs
whether or not the loop ran).

Called by `bmain.cpp` (debug-font fade in/out, boot-card/intro fade out) and
the intro-playback function at 0x231BD8 (which also streams `gsStateFade`).
All boot callers pass one argument; a1 is never materialized, hence the
single-argument source declaration pinned to the two-arg mangled name.

## Match-critical forms
- `vu1ChainHead` is the file's existing double-volatile pointer; the four
  packet stores each re-load the head (self-based absolute `lui/lw`), and the
  head advance goes through the plain alias `vu1ChainHeadStore` whose store
  lands GPREL in the `VU1_syncChain` call delay slot. Same pattern as the
  matched `ResetGsRegisters`.
- `drawFrameCount` (0x15F438, GP window): a PLAIN `extern int` gives the
  self-based absolute load; the `drawFrameCount++` store is scheduled by EGC
  into the `VU1_initChain`/`VU1_sendChain` call delay slots (a `.set
  noreorder` region), where ps2eeas expands the bare pseudo as GPREL16. No
  `.extern`-seeded alias was needed.
- The `(i<<7)/(i+1)` integer division emits EGC's dead zero-divisor guard
  `beql divisor,0; break 0,7` (both edges merge); a plain C division
  reproduces it — same idiom as matched `pause_updateSoundVolume`.
- `func_001F5210` and `func_00122298` are unmangled (stripped-ELF / SDK)
  symbols, declared `extern "C"`.
- `gsStateFade` (0x13CDD0) named in `config/symbols.txt`; an 80-word GS state
  block, near-clone of the 0x13CC90 block used by `func_001F5210` (that
  sibling is still an `INCLUDE_ASM` target and remains unnamed).
