# func_00206BA8 (code/game/menu.cpp)

- Original: 16 bytes at file offset `0x107B28` in `assets/boot_elf.elf`,
  vram `0x00206BA8`. Object-relative `.text+0x230` in `menu.o` (between
  func_00206B78.NON_MATCHING at `+0x200` and func_00206BB8 at `+0x240`).
- Original instruction sequence:
  ```
  lui   $v1, %hi(D_0013D3A6)   # 1400033C
  lbu   $v0, %lo(D_0013D3A6)($v1) # 9062d3a6
  jr    $ra                    # 03e00008
    sltu $v0, $zero, $v0       # 0002102b (delay slot)
  ```
- Semantics: byte-flag predicate `return D_0013D3A6 != 0;` on the unsigned
  data byte at vram `0x0013D3A6` (dlabel in
  `code/_generated/build/data/core.data.data.s:32596`, `.byte 0x00`).
  Ghidra has no function/xrefs at this address; it sits in a gap between the
  matched getters func_00206B98 and func_00206BB8. Read directly from the
  generated asm, like the already-matched siblings func_002069A0/B0/C0 and
  func_00206B88/B98 (same shape, different data byte).
- Replacement:
  ```cpp
  extern u8 D_0013D3A6 __attribute__((section(".data")));

  extern "C" int func_00206BA8(void) {
      return D_0013D3A6 != 0;
  }
  ```
- Verification (mechanical):
  - `nm build/code/game/menu.o`: `func_00206BA8` T at `.text+0x230`; no
    `.NON_MATCHING` alias for this symbol remains.
  - Object `.text[0x230,0x240)` with the two relocations resolved
    (R_MIPS_HI16/R_MIPS_LO16 against `D_0013D3A6`, link value `0x13D3A6`)
    == original ELF slice at file offset `0x107B28`:
    `3c030014 9062d3a6 03e00008 0002102b`.
  - Clean rebuild: `make clean && make split && make -j2`, then
    `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- Follow-up: siblings func_00206BB8 (`D_0013D3A7`) and func_00206BC8
  (`D_0013D3AD`) are the identical shape one line away in menu.cpp; same
  recipe applies.
