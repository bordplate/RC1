# func_001F61F8 (code/game/draw.cpp)

- Original: 8 bytes at file offset `0xF7178` in `assets/boot_elf.elf`, vram
  `0x001F61F8`. Object-relative `.text+0x5670` in `draw.o` (subsegment starts
  at file `0xF1B08`).
- Original instruction sequence:
  ```
  jr      $ra                 # 0800e003
    sw      $zero, -0x7764($gp)  # 9c8880af (delay slot)
  ```
  i.e. sets the gp-relative global `D_0015F49C` (0x166C00 - 0x7764, core.bss)
  to 0 and returns nothing.
- Twin: func_001F61E8 (immediately before, 12 bytes) does the same store with
  value 1 (`addiu $v0,$0,1; jr $ra; sw $v0` — EGC reuses $v0 as the stored
  temp in the return delay slot). The pair is used around `FontPrintWindow`
  calls throughout the pause menu (e.g. FUN_0021a328: set-to-1, draw window,
  set-to-0), so D_0015F49C behaves as a window-text drawing flag.
- Standalone-function evidence: 9 UNCONDITIONAL_CALL xrefs in Ghidra from the
  pause-menu draw code (0x21A9B8, 0x21AAC4, 0x21B4FC, 0x21B558, 0x21B5A8,
  0x21D764, 0x21F4A8, 0x21F778, 0x222BC8). Ghidra shows it called with no
  arguments.
- Replacement (at the old INCLUDE_ASM site, code/game/draw.cpp:108):
  ```cpp
  extern "C" int D_0015F49C;

  extern "C" void func_001F61F8(void) {
      D_0015F49C = 0;
  }
  ```
  plus `D_0015F49C = 0x15f49c;` in config/symbols.txt (GP-relative global
  recipe, same as func_001FF768; the dlabel already existed in
  build/data/lit.lit4.s and `--defsym _gp=0x166c00` is on the ld line).
- EGC 2.95.2 `-G8 -O2`: the void setter with a gp-relative store emits the
  store directly in the return delay slot — first try matched byte-for-byte.
- Verification (mechanical):
  - `build/code/game/draw.o` symtab: `func_001F61F8` at `.text+0x5670`,
    st_size = 8; neighbours contiguous (func_001F61E8 ends at 0x5670,
    func_001F6200 starts at 0x5678).
  - Exactly one reloc in the function: R_MIPS_GPREL16 against D_0015F49C at
    +4 (no other relocations in range).
  - Linked bytes at file offset 0xF7178 == original slice
    `0800e0039c8880af` (full-ELF cmp covers this; see below).
  - draw.o `.text` total unchanged.
  - `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
