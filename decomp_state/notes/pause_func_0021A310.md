# func_0021A310 (code/game/pause.cpp)

- Original: 8 bytes at file offset `0x11B290` in `assets/boot_elf.elf`, vram
  `0x0021A310`. Object-relative `.text+0x1600` in `pause.o`.
- Body identical to func_0021A308:
  ```
  jr      $ra               # 0800e003
    daddu   $v0, $zero,$0   # 2d100000 (delay slot) => returns 0
  ```
- Standalone-function evidence: Ghidra DATA xref from `0x001D1AAC` points
  exactly at it — the second entry of the same small pointer table as
  func_0021A308 (first entry vram 0x1D1AA8 -> 0x21A308, this one 0x1D1AAC ->
  0x21A310; file offset `0x0D2A2C`). See notes/pause_func_0021A308.md for the
  table/identity analysis. Splat vram-space GP = 0x166C00 is irrelevant here
  (no gp-relative accesses).
- Semantics: second "returns false / 0" callback stub in the pause-menu
  callback family, directly adjacent to its twin.
- Replacement (at the old INCLUDE_ASM site, code/game/pause.cpp:31):
  ```cpp
  extern "C" int func_0021A310(void) {
      return 0;
  }
  ```
- EGC 2.95.2 `-G8 -O2`: `return 0;` -> `j $31; move $v0,$0` -> exactly
  `0800e003 2d100000` (same codegen check as func_0021A308).
- Verification (mechanical):
  - `build/code/game/pause.o` symtab: `func_0021A310` at `.text+0x1600`,
    st_size = 8.
  - Object bytes at that offset == original slice `0800e0032d100000`.
  - Neighbours contiguous exactly as original: func_0021A308 ends at 0x1600,
    func_0021A318 starts at `.text+0x1608`.
  - pause.o `.text` total = `0xEA30` == subsegment length (unchanged).
  - `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
