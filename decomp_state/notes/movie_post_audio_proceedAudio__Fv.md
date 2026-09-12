# proceedAudio__Fv (code/game/movie/movie_post_audio.cpp)

- Original: 44 bytes (`0x2C`) at vram `0x23ABA0` (file offset `0x13BB20`),
  symbol `proceedAudio__Fv` (C++ `(void)`), original ELF name.
- Semantics: audio-decoder callback installed by the original unmangled
  movie-decoder API (no direct caller or function-pointer literal for
  0x23ABA0 exists in the boot image; it is registered from an overlay or
  library). Body:
  ```cpp
  void proceedAudio() {
      audioDecSend((_AudioDec*)(movieDecodeBuf + MOVIE_AUDIO_DEC_OFFSET));
  }
  ```
  Sends the movie `_AudioDec` state (at `movieDecodeBuf + 0xD9100`, the
  `MOVIE_AUDIO_DEC_OFFSET` from audiodec.h, the same buffer base as the
  matched `isAudioOK` and `mpegRestartVideoDMA` siblings) to the SPU.
  `void` return: the original keeps the `movieDecodeBuf` base in v0 up to
  the `jal` (the call clobbers it), so an int-returning form would need a
  different register allocation.
- Match: matched on the first standalone probe
  (`decomp_state/probes/movie_post_proceedAudio_v1.cpp`) with
  `-mno-split-addresses`. Default flags give a 2-register `movieDecodeBuf`
  load (`lui v0; lw v1,off(v0)`) with the offset `lui` scheduled first; the
  flag turns the named `.data` load into the single self-based pseudo
  (`lui v0; lw v0,off(v0)`) hoisted before the prologue, exactly as in the
  original. The argument constant 0xD9100 materializes as
  `lui a0,0xD; ori a0,a0,0x9100` (unsigned split) straddling the prologue
  (lui before `addiu sp,-0x10`, ori after), the frame is 0x10 with
  `sq/lq ra` at 0(sp), and the final `addu a0,v0,a0` lands in the `jal`
  delay slot.
- Flags: the flag breaks ErrMessage's matched codegen in movie_post.cpp, so
  a Splat boundary at 0x13BB20 puts proceedAudio in its own TU
  `movie_post_audio.cpp` with `PRIVATE_COMPILE_FLAGS = -mno-split-addresses`
  (same pattern as movie_mid.o / videodec_post.o).
- Verification: probe `match: true` (44/44 bytes, word-for-word); full
  `make split && make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf`
  byte-for-byte; decomp count 710 -> 709.
