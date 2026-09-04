# func_00206BB8 (code/game/menu.cpp)

- Original: 16 bytes at file offset `0x107B38` in `assets/boot_elf.elf`,
  vram `0x00206BB8`. Object-relative `.text+0x240` in `menu.o` (between
  func_00206BA8 at `+0x230` and func_00206BC8 at `+0x250`).
- Original instruction sequence (big-endian words):
  ```
  lui   $v1, %hi(D_0013D3A7)   # 3c030014
  lbu   $v0, %lo(D_0013D3A7)($v1) # 9062d3a7
  jr    $ra                    # 03e00008
    sltu $v0, $zero, $v0       # 0002102b (delay slot)
  ```
- Semantics: byte-flag predicate `return D_0013D3A7 != 0;` on the unsigned
  data byte at vram `0x0013D3A7`. Same family as matched func_002069A0/B0/C0,
  func_00206B88/B98/BA8 (identical shape, different data byte). Ghidra has no
  function at 0x206BB8; single DATA xref from `0x0019ff1c` (function-pointer
  table entry), so semantics read directly from the generated asm, as with the
  siblings.
- Replacement:
  ```cpp
  extern u8 D_0013D3A7 __attribute__((section(".data")));

  extern "C" int func_00206BB8(void) {
      return D_0013D3A7 != 0;
  }
  ```
- Verification (mechanical):
  - `nm build/code/game/menu.o`: `func_00206BB8` T at `.text+0x240`; no
    `.NON_MATCHING` alias remains. Only relocations in the range are
    R_MIPS_HI16 @0x240 and R_MIPS_LO16 @0x244 against `D_0013D3A7`.
  - Object `.text[0x240,0x250)` with those relocations resolved
    (link value `0x13D3A7` from build/undefined_syms_auto.txt) == original ELF
    slice at file offset `0x107B38`:
    `3c030014 9062d3a7 03e00008 0002102b` (LE bytes
    `1400033c a7d36290 0800e003 2b100200`). menu.o `.text` size unchanged
    (`0x26b8`).
  - Clean rebuild: `make clean && make split && make -j2`, then
    `cmp build/boot_elf.elf assets/boot_elf.elf` passes. decomp_status count
    854 -> 853; `func_00206BB8.s` moved from nonmatchings to matchings by
    split.
- Follow-up: func_00206BC8 (`D_0013D3AD`) at menu.cpp:60 is the identical
  shape one line away; same recipe applies. (func_00206BD8 next is not the
  same shape — inspect before assuming.)
