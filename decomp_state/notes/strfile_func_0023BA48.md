# func_0023BA48 (code/game/movie/strfile.cpp)

- Original: 16 bytes at file offset `0x13C9C8` in `assets/boot_elf.elf`,
  vram `0x0023BA48`. Object-relative `.text+0x00` in `strfile.o` (first
  function in the file; neighbours strFileDelete__FP7StrFile at `+0x10`,
  func_0023BA60 at `+0x18`).
- Original instruction sequence:
  ```
  addiu   $v0, $zero, 1      # 01000224
  sw      $a2, 0($a0)        # 000086ac
  jr      $ra                # 0800e003
    sw     $a1, 4($a0)       # 040085ac (delay slot)
  ```
- Semantics (Ghidra FUN_0023ba48): `*self = arg2; self[1] = arg1; return 1;`
  Sets both StrFile header words and returns success.
- Sole caller: `initAll__Fiii` (code/game/movie/movie.cpp, nonmatching) at
  vram `0x0023A9BC`:
  ```
  move a1, s2      # initAll arg0 (saved at entry via move s2,a0)
  addu a0, v1,a0   # &DAT_0016120c[0xd9040] = the resident StrFile instance
  jal  func_0023BA48
    move a2, s3    # initAll arg1 (saved via move s3,a1)
  ```
  so `offset = initAll arg0`, `frameCount = initAll arg1`. Result is tested
  (`bne v0,zero`) and on failure `func_001e93b0(0x1E8AF0)` (error handler)
  runs; the bool result is also what initAll returns near its end.
- Field usage cross-evidence:
  - offset (+4): stream read position. func_0023BA60 reads at `*(self+4)`
    via FUN_00121450 and advances it by the chunk size when flag==0.
  - frameCount (+0): consumed as a counter in readMpeg (FUN_0023a460):
    `iVar = *(int*)strFile`, loop guard `ivar < 5`, decremented per decoded
    video-frame batch. Names are best-effort; offsets/types are what parity
    fixes, and both are plain 32-bit words (sw).
- Replacement:
  ```cpp
  typedef struct StrFile {
      u32 frameCount;
      u32 offset;
  } StrFile;

  extern "C" int func_0023BA48(StrFile* self, int offset, int frameCount) {
      self->offset = offset;
      self->frameCount = frameCount;
      return 1;
  }
  ```
- Codegen pitfall: EGC 2.95.2 (-G8 -O2 -ffast-math -fno-exceptions) emits the
  two stores in SOURCE order with the last one in the `jr $ra` delay slot, and
  hoists `addiu $v0,$0,1` to the top. The reversed assignment order
  (`frameCount = ...; offset = ...;`) produces a mirrored store schedule
  (sw a1,4(a0) before the jr) and does NOT match. Keep
  `self->offset` first, `self->frameCount` second.
- Verification (mechanical):
  - Clean rebuild: `make clean && make split && make -j2`, then
    `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
  - `nm build/code/game/movie/strfile.o`: `func_0023BA48` T at `.text+0x0`,
    strFileDelete__FP7StrFile at `+0x10`, func_0023BA60 at `+0x18` (exact
    original layout); no `.NON_MATCHING` alias for this function.
  - Object `.text[0x00,0x10)` == `01000224 000086ac 0800e003 040085ac` ==
    original ELF slice at file offset `0x13C9C8`.
  - `.rel.text` has no entries in `[0x00,0x10)` (only the two R_MIPS_26 jal
    relocations inside func_0023BA60 at 0x64/0x84), so raw object bytes equal
    linked output for this range.
