# mpegNodata (code/game/movie/videodec.cpp)

- Original: 56 bytes (`0x38`) at vram `0x0023D0A8` (file offset `0x13E028`).
- Matched implementation (refactored 2026-09-16, in its own TU
  `code/game/movie/videodec_nodata.cpp`):
  ```cpp
  void viBufAddDMA(ViBuf* buf);
  extern "C" void switchThread(void);

  int mpegNodata(struct sceMpeg* mpeg, struct sceMpegCbData* cbData, void* user) {
      switchThread();
      viBufAddDMA((ViBuf*)(movieDecodeBuf + MOVIE_VIBUF_OFFSET));
      return 1;
  }
  ```
- This is the SCE mpeg "no data" callback (type `videoDecCallback`,
  `int (*)(sceMpeg*, sceMpegCbData*, void*)`). It yields the thread
  (`switchThread`), then arms the video DMA for the ViBuf that lives at
  `base + 0xD9090`, and returns 1. The three callback parameters are unused.
- `0x16120C` is a 32-bit base for the movie/streaming data (Ghidra
  `DAT_0016120c` / `iGpffffa60c`). Sibling offsets used by the movie code:
  `+0xD9090` (this ViBuf), `+0xD9100` (`_AudioDec`, see isAudioOK /
  proceedAudio), `+0xD9040`, `+0xD9048`, `+0xD9168`. Written once at
  `0x23A454` (in `func_0023A3B8`) and zeroed at teardown. Accessed here
  through the named `movieDecodeBuf` symbol (`.data` extern in audiodec.h);
  the load is the absolute self-based `lui/lw` RELOAD into `$v0` because the
  TU compiles with `-mno-split-addresses`.
- `viBufAddDMA` (vram `0x0023BF70`, `viBufAddDMA__FP5ViBuf`) and
  `switchThread` (vram `0x0023A770`, C linkage, defined in movie.cpp) are the
  two callees; both still nonmatching / C respectively.

## EGC findings

- The constant-cast load `*(int*)0x16120C` matches: EGC emits the absolute
  `lui $v0; lw $v0, off($v0)` (RELOAD, value in `$v0`), then `addu $a0,$v0,$a0`
  in the `jal viBufAddDMA` delay slot. This is the same load/register shape as
  the original.
- This function has TWO calls (`switchThread`, then `viBufAddDMA`). The
  standard prologue-first schedule matches byte-for-byte. This is the key
  difference from `isAudioOK` / `proceedAudio__Fv` (below), which have a single
  call whose argument is the loaded value: in those, the ORIGINAL hoists the
  load BEFORE the prologue, and no EGC source form reproduces that (the cast
  form keeps the prologue first; the `section(".data")` symbol form hoists but
  allocates the loaded value to `$v1` instead of reusing `$v0` — a 3-word gap).
  See notes/movie_isAudioOK.md. The extra leading no-arg call in mpegNodata is
  what lets the cast form match here.

## Verification (mechanical)

- `decomp_probe.py` with a standalone candidate
  (`--define viBufAddDMA__FP5ViBuf=0x23bf70`): 56/56 bytes, 0 differences.
- `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf`: pass.
- `python3 tools/decomp_status.py --count`: 733 -> 732.

## Refactor to named symbol (2026-09-16)

- The constant cast was replaced with `movieDecodeBuf + MOVIE_VIBUF_OFFSET`
  (the `u8*` `.data` extern in audiodec.h; pointer arithmetic over the base is
  the same value the cast computed).
- Under the TU's default (address-splitting-on) flags the named form does NOT
  match: EGC splits the load into `lui $v0; lw $v1, 0x120c($v0)` (two
  registers, value in `$v1`) and re-schedules it around the constant setup —
  4 words differ. With `-mno-split-addresses` the named form emits the single
  self-based pseudo that ps2eeas expands to the original `lui $v0; lw $v0`.
- Adding the flag to the whole videodec TU broke the sibling
  `mpegError__FP7sceMpegP18sceMpegCbDataErrorPv` (0x23D080, 8 words): its
  `videoErrorMessage` address setup (`lui $a0; addiu $v0`) reordered around
  `sq $ra`. So, per the Splat-boundary-split policy, the videodec segment
  (0x13DA48-0x13E060) was cut at file offset 0x13E028 into
  `code/game/movie/videodec_nodata.cpp` carrying `-mno-split-addresses`;
  `videodec.o` keeps default splitting and still matches.
- mpegNodata is the only function in the new segment, so the flag cannot
  disturb anything else there.
- Verification: mpegNodata objdump 14/14 words identical to the original;
  `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf`: pass.
