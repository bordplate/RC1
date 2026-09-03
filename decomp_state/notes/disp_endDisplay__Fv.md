# endDisplay__Fv (code/game/movie/disp.cpp)

- Original: 16 bytes (`0x10`) at file offset `0x13C564` in
  `assets/boot_elf.elf`, vram `0x0023B5E0`. Ghidra FUN_0023b5e0.
  Instruction sequence:
  ```
  lui     $at, 0x16             # 3c010016
  sw      $zero, 0x11e0($at)    # ac2011e0
  jr      $ra                   # 03e00008
  nop                           # 00000000
  ```
- Semantics: clears the movie "display active" flag at vram `0x1611E0`
  (Ghidra DAT_001611e0, a word inside the original lit section
  `0x15EF00-0x161228`, dlabel `D_001611E0` in
  `_generated/build/data/lit.lit4.s`). Xrefs of that word:
  - `startDisplay__Fi` (FUN_0023b590, this file): after waiting for the
    display sync call, sets it to 1 and zeroes frame counter DAT_001611e4.
  - vblank handler FUN_0023b3d8: only runs its per-vblank display work
    (increment DAT_001611e4, VU/GS field-state handling via
    DAT_001611ec/DAT_001611f0) while the flag is nonzero.
  - endDisplay itself is called by the movie playback loop FUN_0023a460
    right after the last frame, before tearing the decoder down.
- Codegen note (why a raw address): `0x1611E0` is within ±32k of gp
  (`gp = 0x166C00`, distance `0x5A20`), so a plain
  `extern "C" int D_001611E0; D_001611E0 = 0;` compiles to a single
  gp-relative `sw $zero, off($gp)` — no match. With
  `__attribute__((section(".lit4")))` (or `.data`) it compiles to
  `lui $v0 / jr $ra / sw $zero, lo($v0) [delay] / nop` — right bytes in
  the wrong register/order. EEGCC 2.95.2 emits the original `$at`-based
  sequence only for a store through a pointer cast to a compile-time
  constant address at -O2 (verified with standalone test objects; a
  `volatile` cast instead yields `lui $v0 / ori $v0 / sw / jr`, also a
  miss). Hence the constant-address form below.

## Replacement

```cpp
void endDisplay(void) {
    *(int*)0x1611E0 = 0;
}
```

Plain C++ free function; EGC old-ABI mangling gives `endDisplay__Fv`.
The write is a full-word zero store, matching the original `sw $zero`.

## Verification (mechanical)

- `make -j2` builds cleanly.
- `cmp build/boot_elf.elf assets/boot_elf.elf` passes (byte-for-byte).
- Built region file offset `0x13C564..0x13C574` =
  `3c010016 ac2011e0 03e00008 00000000`, identical to the original slice.
- `tools/decomp_status.py --count`: 867 -> 866.
