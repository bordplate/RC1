# func_0021D1F0 (code/game/pause.cpp)

- Original: 8 bytes at file offset `0x11E170` in `assets/boot_elf.elf`, vram
  `0x0021D1F0`. Object-relative `.text+0x44E0` in `pause.o`.
- Body: `jr $ra; daddu $v0,$zero,$0` -> returns 0. Member of the pause-menu
  "return false" callback-stub family (identity analysis and EGC codegen check
  in notes/pause_func_0021A308.md).
- Standalone evidence: Ghidra DATA xref from `0x001D0364` points exactly at it.
- Replacement (pause.cpp, at the old INCLUDE_ASM site):
  ```cpp
  extern "C" int func_0021D1F0(void) {
      return 0;
  }
  ```
- Verification (mechanical): pause.o symbol at `.text+0x44E0` (0x11E170 -
  seg 0x119C90), st_size = 8; neighbour func_0021D1F8 contiguous at 0x44E8;
  `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
