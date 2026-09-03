# func_001F61E8 (code/game/draw.cpp)

- Original: 12 bytes (`0xC`) at file offset `0xF7168` in `assets/boot_elf.elf`,
  vram `0x001F61E8`. Object-relative `.text+0x5660` in `draw.o`. The nop at
  `0xF7174` after the function is section alignment filler (next function is
  already 8-aligned), not part of it.
- Original instruction sequence:
  ```
  addiu   $v0, $zero, 0x1   # 01000224
  jr      $ra               # 0800e003
    sw      $v0, -0x7764($gp)  # 9c8882af (delay slot)
  ```
  i.e. sets gp-relative global `D_0015F49C` to 1, returns nothing. EGC reuses
  `$v0` as the stored temp in the return delay slot (void function).
- Semantics: the "set" half of the func_001F61E8/func_001F61F8 pair — a
  window-text drawing flag for `FontPrintWindow` (see notes/draw_func_001F61F8.md).
- Standalone-function evidence: real function entry with unconditional-call
  xrefs from the pause-menu draw code (Ghidra; see sibling note), sits directly
  before func_001F61F8 which shares its global.
- Replacement (at the old INCLUDE_ASM site, code/game/draw.cpp:106):
  ```cpp
  extern "C" int D_0015F49C;

  extern "C" void func_001F61E8(void) {
      D_0015F49C = 1;
  }
  ```
  (the `D_0015F49C` declaration was moved above the first use when this
  function was added; symbols.txt entry and `_gp` defsym from the sibling
  commit cover linking.)
- EGC 2.95.2 `-G8 -O2`: void gp-relative setter emits `addiu $v0,$0,1; j $31;
  sw $v0,off(gp)` — first try matched byte-for-byte (same codegen class as the
  videoDecAbort finding).
- Verification (mechanical):
  - `build/code/game/draw.o` symtab: `func_001F61E8` at `.text+0x5660`,
    st_size = 0xC; contiguous with func_001F61F8 at 0x5670.
  - Relocs: R_MIPS_GPREL16 against D_0015F49C at +8 (the store); pre-reloc
    object bytes `01000224 0800e003 000082af`, linked io filled to 0x889C.
  - Linked bytes at file offset 0xF7168 == original slice
    `010002240800e0039c8882af` (full-ELF cmp).
  - draw.o `.text` total = `0x6F10` == subsegment length (unchanged).
  - `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
