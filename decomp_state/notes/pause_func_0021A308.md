# func_0021A308 (code/game/pause.cpp)

- Original: 8 bytes at file offset `0x11B288` in `assets/boot_elf.elf`, vram
  `0x0021A308`. Object-relative `.text+0x15F8` in `pause.o` (subsegment starts
  at file `0x119C90`, ends at `0x1286C0`, length `0xEA30`).
- Original instruction sequence:
  ```
  jr      $ra               # 0800e003
    daddu   $v0, $zero,$0   # 2d100000 (delay slot) => returns 0
  ```
- Identity question: the boot ELF has **no symbol table at all** (`nm`/readelf
  report no symbols), so splat named this `func_0021A308` via its flow-based
  splitting after the preceding function's epilogue (func_0021A1E0 ends with
  `jr $ra; addiu $sp,$sp,0x40` at 0x21A2FC/0x21A300 + nop filler at 0x21A304).
  It is nonetheless a standalone callable, not a mid-function fragment:
  - Ghidra defines no function there (region folded), but has a DATA xref from
    `0x001D1AA8` pointing exactly at it.
  - That word sits in core.data as part of a small pointer table:
    `[ ... , 0x2212B8, 0x21A308, 0, ... ]` (file offset `0x0D2A28`). The
    adjacent entry 0x2212B8 is unambiguously a function start
    (Ghidra FUN_002212b8), so table entries are code pointers to function
    entries. No code in the boot ELF loads this table by lui/addiu (scanned);
    it is likely consumed via indexed loads or by overlay code, so its exact
    consumer could not be pinned down.
- Semantics: a "returns false / 0" callback (predicate-style stub), consistent
  with sibling entries in the pause-menu callback family. The next two splat
  splits are `func_0021A310` (identical `return 0` body) and `func_0021A318`
  (`return 0; this[+0x44] = -1;` setter with store in the jr delay slot).
- Replacement (at the old INCLUDE_ASM site, code/game/pause.cpp:29):
  ```cpp
  extern "C" int func_0021A308(void) {
      return 0;
  }
  ```
- EGC 2.95.2 codegen check (`-G8 -O2 -ffast-math -fno-exceptions`, standalone
  `-S` test): `return 0;` emits `j $31; move $v0,$0`, which assembles to exactly
  `0800e003 2d100000`. (An empty function emits only `j $31` + implicit delay
  slot nop, so the explicit zeroing is required.)
- Verification (mechanical):
  - `build/code/game/pause.o` symtab: `func_0021A308` at `.text+0x15F8`,
    st_size = 8.
  - Object bytes at that offset == original slice `0800e0032d100000`.
  - Neighbours contiguous exactly as original: func_0021A310 at `.text+0x1600`,
    func_0021A318 at `.text+0x1608`.
  - pause.o `.text` total = `0xEA30` == subsegment length (unchanged).
  - `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
