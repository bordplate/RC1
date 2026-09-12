# mpegRestartVideoDMA (vram 0x23D110, file 0x13E090, 48 bytes / 12 words) — MATCHED 2026-09-12

C++ free function in code/game/movie/videodec_post.cpp (line 12).
Original Splat placeholder `func_0023D110`. C++ name `mpegRestartVideoDMA`,
mangled `mpegRestartVideoDMA__Fv` (zero-arg; the body ignores its callback
arguments). Added to config/symbols.txt as `mpegRestartVideoDMA__Fv = 0x23d110;`.

## Semantics (confirmed)

Video-decoder callback, table slot 3. `videoDecCreate` (FUN_0023cac8, still
INCLUDE_ASM) installs it at 0x23CB54 via `videoDecRegisterCallback(self, 3,
0x23D110, 0)` (a reloc to `mpegRestartVideoDMA__Fv`). It restarts the movie
video ViBuf DMA:

    viBufRestartDMA((ViBuf*)(movieDecodeBuf + MOVIE_VIBUF_OFFSET));  // 0xD9090
    return 1;

It ignores its callback arguments (no arg register is live; a0 is clobbered for
the 0xD9090 constant) and returns 1. `viBufRestartDMA` (0x23C280) is a large
INCLUDE_ASM ViBuf helper that returns 1; its return is discarded here. The
sibling callback mpegNodata (slot 1, 0x23D0A8) uses the same `movieDecodeBuf +
0xD9090` ViBuf but calls viBufAddDMA and has a preceding switchThread() call.
Ghidra FUN_0023d110: a single `viBufRestartDMA(*(movieDecodeBuf)+0xd9090);
return 1;`.

## Exact original (12 words)

```
0:   3C020016  lui   v0, 0x16              # %hi(movieDecodeBuf=0x16120C)
4:   8C42120C  lw    v0, 0x120C(v0)        # VALUE ENDS IN v0
8:   3C04000D  lui   a0, 0xD               # 0xD9090 hi
C:   27BDFFF0  addiu sp, sp, -0x10         # prologue (4th insn!)
10:  34849090  ori   a0, a0, 0x9090        # 0xD9090 lo
14:  7FBF0000  sq    ra, 0(sp)
18:  0C08F0A0  jal   viBufRestartDMA__FP5ViBuf
1C:  00442021  addu  a0, v0, a0            # arg = base + 0xD9090 (jal delay slot)
20:  7BBF0000  lq    ra, 0(sp)
24:  24020001  li    v0, 1
28:  03E00008  jr    ra
2C:  27BD0010  addiu sp, sp, 0x10
```

The movieDecodeBuf absolute load pair (lui v0; lw v0) AND the constant-hi
(lui a0) are hoisted BEFORE the prologue; the constant-lo (ori a0) and ra-save
(sq ra) are AFTER it. This is the same hoist-before-prologue codegen as
isAudioOK (movie_isAudioOK.md).

## Why -mno-split-addresses (and the Splat split)

movieDecodeBuf (0x16120C) is a named in-window pointer global. To reproduce the
original's single self-based absolute load hoisted before the prologue, the C
form needs `movieDecodeBuf` declared `__attribute__((section(".data")))` AND the
TU compiled with `-mno-split-addresses` (see AGENTS.md 2026-09-11/09-12 notes).
Verified by probe:

- plain extern / no flag          -> GP-relative or split load; prologue FIRST (wrong).
- constant cast `*(int*)0x16120C`, no flag -> prologue FIRST (wrong; matches
  mpegNodata's shape only because that one has a preceding call).
- named `.data` symbol + -mno-split-addresses -> `lw v0,movieDecodeBuf` pseudo
  (expands to lui v0; lw v0), hoisted BEFORE the prologue. Byte-identical to the
  original (objdump of the probe .o matched all 12 words; only relocs pending).

## Why the Splat range split (videodec_post.cpp)

Adding -mno-split-addresses to the WHOLE videodec.o breaks mpegError (0x23D080,
same file): mpegError loads the `videoErrorMessage` global and the original
interleaves `lui a0; sq ra; addiu a0`, but the flag makes EGC emit `lui a0;
addiu a0; sq ra` (8-byte parity diff at 0x23D088). mpegNodata (constant cast,
no global load) is unaffected by the flag (verified identical with/without). So
the flag cannot be shared with the rest of videodec.cpp.

Per the AGENTS.md conflicting-flags procedure, a Splat boundary was added at
file 0x13E090 (VA 0x23D110, the start of mpegRestartVideoDMA) in
config/RC1.yaml, creating a new TU `game/movie/videodec_post` covering
[0x23D110, 0x23D190) = mpegRestartVideoDMA + func_0023D140. Makefile:
`$(OBJ_DIR)/game/movie/videodec_post.o` gets `-mno-split-addresses` (joined to
the existing movie_mid.o rule); videodec.o keeps defaults. func_0023D140 is an
INCLUDE_ASM so the flag is a no-op for it.

## Shared declaration refactor

movieDecodeBuf was moved from a local declaration in movie_mid.cpp into
include/audiodec.h (its natural home, next to MOVIE_AUDIO_DEC_OFFSET) so both
movie_mid.cpp (isAudioOK) and videodec_post.cpp (mpegRestartVideoDMA) share it.
MOVIE_VIBUF_OFFSET (0xD9090) was added there too. mpegNodata still uses the
constant-cast `*(int*)0x16120C + 0xD9090` (pre-existing; left as-is to avoid
touching a matched function).

## Verification

- Probe .o objdump: 12/12 words match the original (relocs pending).
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf`: MATCH.
- mpegError and mpegNodata re-verified intact (no parity diff).
