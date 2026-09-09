# videoDecIsFlushed (code/game/movie/videodec.cpp)

- Original: 72 bytes (`0x48`) at vram `0x0023CDE0` (file offset `0x13DD60`).
- Matched implementation:
  ```cpp
  extern "C" unsigned int func_0012BA58(VideoDec* self);

  int videoDecIsFlushed(VideoDec* self) {
      int ret = 0;
      if (videoDecInputCount(self) == 0)
          ret = func_0012BA58(self) > 0;
      return ret;
  }
  ```
  "Flushed" means the input ViBuf is empty AND `func_0012BA58(self) != 0`.
- `func_0012BA58` (vram `0x0012BA58`, lives in the unsplit `sce/lib` asm blob,
  no ELF symbol):
  ```
  lw    $v1, 0x40($a0)
  lw    $v0, 4($v1)
  jr    $ra
    sltiu $v0, $v0, 1
  ```
  Returns `(*(u32**)(mpeg + 0x40))[1] == 0`. VideoDec+0x40 points into the
  embedded SCE mpeg struct (created through `func_0012AEC8`, registered by
  `videoDecSetStream`); that sibling uses the same +0x40 sub-struct for its
  stream table at +0x44 with count at +0x48. Kept as `func_0012BA58` since
  the field's meaning is not established.
- Only caller in the ELF: `readMpeg` at vram `0x0023A6E8`
  (`readMpeg__FP8VideoDecP7ReadBufP7StrFile` in code/game/movie/movie.cpp):
  a wait loop spinning while `!videoDecIsFlushed(dec) && state != 3`.

## EGC findings

- The callee return type must be UNSIGNED. The original comparison is
  `sltu $s1, $zero, $v0` (word `0002882B`). Declaring `func_0012BA58` as
  returning `int` emits `slt` (`0002882A`) — a single-word diff;
  `unsigned int` matches. The SCE-side header evidently declared the
  return unsigned.
- The `int ret = 0;` local takes s1 as its home: prologue saves
  `sq s0,0; sq s1,0x10; sq ra,0x20` (s1 saved before it is set),
  `s1 = 0` lands in the first `jal` delay slot, and the zero return
  `v0 = s1` lands in the `bnez` delay slot. An early-return form
  (`if (videoDecInputCount(self)) return 0; return f(self) > 0;`) would
  not save/restore s1 and cannot match.

## Verification (mechanical)

- `decomp_probe.py` with `decomp_state/probes/videodec_isflushed.cpp`
  (`--define func_0012BA58=0x12BA58`): 72/72 bytes, 0 differences.
- `objdump` of `build/boot_elf.elf` at `0x23CDE0`: all 18 words identical
  to the original, `jal 0x23cce0` (videoDecInputCount) and
  `jal 0x12ba58` resolved correctly at link.
- `make` + `cmp build/boot_elf.elf assets/boot_elf.elf`: pass.
- `python3 tools/decomp_status.py --count`: 734 -> 733.
