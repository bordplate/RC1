# audioDecDelete__FP9_AudioDec (vram 0x23AC90, file 0x13BC10, 32 bytes) — MATCHED 2026-09-05

```
addiu sp,sp,-0x10
sq    ra,0(sp)
jal   snd_CloseMovieSound      ; 0x12F0E0
nop
lq    ra,0(sp)
li    v0,1
jr    ra
addiu sp,sp,0x10               ; delay slot
```

## Identity / context

Movie-audio teardown. Closes the movie SPU stream and reports success:
`snd_CloseMovieSound(); return 1;`

- `snd_CloseMovieSound` (matched, code/989snd/ee/989snd.c:119, vram 0x12F0E0):
  `snd_SendIOPCommandAndWait(0x3C, 0, 0)` — IOP cmd 0x3C = close movie sound.
- Sole caller: the movie end sequence at 0x23AB20 (Ghidra FUN_0023aa68 body),
  which tears down the movie struct at D_0016120C in order: video dec
  (0xD9048 via func_0023CC38), audio dec (0xD9100 via this function), vo buf
  (0xD9040 via func_0023BA58), then removes the DMA/INTC handlers on channel 2
  and clears the DMAC bit. The `_AudioDec` is embedded in that struct at
  +0xD9100; the argument is unused by this function.

## Codegen findings

- Pure C++ free function `int audioDecDelete(_AudioDec* self)` in
  audiodec.cpp; EGC old-cfront mangling yields `audioDecDelete__FP9_AudioDec`
  exactly (same recipe as voBufDelete / strFileDelete).
- First C++ file to reference an `snd_*` symbol: added
  `extern "C" void snd_CloseMovieSound(void);` at the call site (no header
  exists for 989snd; precedent for inline extern "C" decls: movie.cpp
  STUB_printf/ErrMessage).
- The `void` prototype of the callee is what produces the `nop` in the `jal`
  delay slot (no argument setup). If the prototype had a parameter, EGC would
  emit `daddu a0,zero,zero` in the slot (inverse of the stash_func_00232CE0
  finding, where a missing parameter broke the match).
- `li v0,1` is emitted AFTER `lq ra,0(sp)` (source order: call, then return);
  EGC does not hoist the return-value setup above the epilogue reload for
  this shape. The 0x10 frame with sq/lq $ra at 0(sp) is the standard
  call-making leaf shape.

## Verification

- `mipsel-linux-gnu-objdump -d build/code/game/movie/audiodec.o`: symbol
  `T audioDecDelete__FP9_AudioDec`, 8 words identical to the original
  (0x27BDFFE0 0x7FBF0000 0x0C04BC38 0x00000000 0x7BBF0000 0x24020001
  0x03E00008 0x27BD0010). The jal is a `R_MIPS_26 snd_CloseMovieSound`
  relocation; the final link resolves it to the original target word.
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf`: byte-for-byte OK.
- Nonmatching count 817 -> 816.
