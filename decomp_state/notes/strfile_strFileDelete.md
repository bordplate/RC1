# strFileDelete__FP7StrFile (code/game/movie/strfile.cpp)

- Original: 8 bytes (`0x8`) at file offset `0x13C9D8` in `assets/boot_elf.elf`,
  vram `0x0023BA58`. Object-relative `.text+0x10` in `strfile.o`, between
  `func_0023BA48` (`.text+0x00`) and `func_0023BA60` (`.text+0x18`).
- Semantics: the StrFile "delete" hook. Does nothing but return success (1).
  Consistent with its movie-subsystem siblings: `videoDecDelete__FP8VideoDec`
  and `audioDecDelete__FP9_AudioDec` both end in `addiu $v0,$0,1; jr $ra`.
- Original instruction sequence:
  ```
  jr      $ra
    addiu $v0, $0, 1     (delay slot)
  ```
- Ghidra decompile of `FUN_0023ba58`: `return 1;` (no parameters used).
- Sole xref: UNCONDITIONAL_CALL from vram `0x0023AB38`, inside
  `termAll__Fv` (code/game/movie/movie.cpp), right after the calls to
  `videoDecDelete__FP8VideoDec` and `audioDecDelete__FP9_AudioDec`, with the
  StrFile object pointer in `$a0`. The class name is corroborated by the
  existing mangled symbol `readMpeg__FP8VideoDecP7ReadBufP7StrFile`
  (`P7StrFile`).

## Naming (old-GCC C++ mangling)

Per the established finding in vobuf_voBufDelete.md, EGC 2.95.2 uses the old
GCC pre-Itanium ABI: the binary symbol `strFileDelete__FP7StrFile` is exactly
the compiler-mangled name of the free function `int strFileDelete(StrFile*)`.
Defined as plain C++ (no extern "C"), per project guidance.

The symbol was named in `config/symbols.txt`
(`strFileDelete__FP7StrFile = 0x0023ba58;`, new `// movie/strfile.cpp` block)
and `make split` regenerated `termAll__Fv.s` so its `jal` now targets the new
name. Splat does not delete stale nonmatching `.s` files, so
`func_0023BA58.s` remains on disk as an orphan (gitignored); it is no longer
emitted because its spim name no longer appears in strfile.cpp's INCLUDE_ASM
set.

## C candidate

```cpp
typedef struct StrFile {
    u8 _pad[8];
} StrFile;

int strFileDelete(StrFile* self) {
    return 1;
}
```

The `StrFile` typedef is declared locally (like `VoBuf` in vobuf.cpp); the
layout is left as padding because this function touches no fields. The
sibling `func_0023BA48` writes two words at offsets 0 and 4, so the real
struct has at least that; expand when decompiling it.

## Verification (mechanical)

- Clean verification: `make clean && make split && make -j2`, then
  `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- `nm build/code/game/movie/strfile.o`: `T strFileDelete__FP7StrFile` at
  `.text+0x10` (mangling matches the original symbol exactly), neighbors
  `func_0023BA48` at 0x00 and `func_0023BA60` at 0x18 unchanged.
- Object slice bytes `0800e003 01000224` identical to the original ELF slice
  at file offset `0x13C9D8`.
- `objdump -r strfile.o`: no relocations in `[0x10, 0x18)` (only the two
  jal relocations inside func_0023BA60 at 0x64/0x84).
- Because the rename changed `termAll__Fv.s` but the Makefile does not track
  generated-asm dependencies, `build/code/game/movie/movie.o` had to be
  deleted before relinking (stale object still referenced `func_0023BA58`).
  Clean rebuild confirms this is not a problem from scratch.

## Remaining strfile.cpp targets

- `func_0023BA48` (16 bytes): MATCHED 2026-09-04 — stores two u32s at StrFile
  offsets 0/4 and returns 1 (see strfile_func_0023BA48.md).
- `func_0023BA60` (0x98 bytes): real workhorse — builds a 3-byte buffer
  (`sb $v0,0(sp); sb $0,1(sp); sb $0,2(sp)` after `addiu $v0,$0,0x64`), calls
  `func_00121450(*(u32*)(a0+4), a2>>11, a1, buf)`, and if `a3==0` advances
  `*(a0+4)` by that shift and calls `func_00120C30(0)`; needs those file
  manager symbols named first.

## Process note for symbol renames

Renaming a target in `config/symbols.txt` + `make split` rewrites every
generated asm reference to it, but already-built `.o` files of the referencing
sources go stale (the Makefile compiles `code/%.cpp` without dependency on the
included `_generated/*.s`). When renaming a symbol that other nonmatching
functions call by name, delete the referencing objects (or run the clean
verification) before trusting the link.
