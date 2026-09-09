# mpegNodata (code/game/movie/videodec.cpp)

- Original: 56 bytes (`0x38`) at vram `0x0023D0A8` (file offset `0x13E028`).
- Matched implementation:
  ```cpp
  void viBufAddDMA(ViBuf* buf);
  extern "C" void switchThread(void);

  int mpegNodata(struct sceMpeg* mpeg, struct sceMpegCbData* cbData, void* user) {
      switchThread();
      viBufAddDMA((ViBuf*)(*(int*)0x16120C + 0xD9090));
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
  `0x23A454` (in `func_0023A3B8`) and zeroed at teardown. Accessed here with
  the constant-cast idiom to force the absolute `lui/lw` (RELOAD into `$v0`).
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
