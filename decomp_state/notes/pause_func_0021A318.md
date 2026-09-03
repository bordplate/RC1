# func_0021A318 (code/game/pause.cpp)

- Original: 16 bytes (`0x10`) at file offset `0x11B298` in `assets/boot_elf.elf`,
  vram `0x0021A318`. Object-relative `.text+0x1608` in `pause.o`.
- Original instruction sequence:
  ```
  addiu   $v1, $zero, -0x1    # ffff0324
  daddu   $v0, $zero,$0       # 2d100000 (returns 0)
  jr      $ra                 # 0800e003
    sw      $v1, 0x44($a0)    # 440083ac (delay slot)
  ```
- Semantics: stores `-1` at `*(a0 + 0x44)` and returns 0. A "reset/cancel"
  callback on the pause-menu state object (field at offset 0x44 holds a
  counter/index — cf. FUN_0021a328 operating heavily on +0x44/+0x48/+0x4C).
- Standalone-function evidence: 33 Ghidra DATA xrefs point exactly at it
  (0x001CE878 .. 0x001D4D18) — entries in many pause-menu item descriptor
  tables; a widely used callback, certainly a function entry. It sits directly
  after the func_0021A308/func_0021A310 twin pair (see their notes).
- Replacement (at the old INCLUDE_ASM site, code/game/pause.cpp:33):
  ```cpp
  extern "C" int func_0021A318(int* p) {
      p[0x11] = -1;
      return 0;
  }
  ```
- EGC 2.95.2 codegen check (`-G8 -O2 -ffast-math -fno-exceptions`, standalone
  `-S` test of `int f(int* p){ p[0x11] = -1; return 0; }`): emits exactly
  `li $3,-1; move $2,$0; j $31; sw $3,68($4)` ->
  `ffff0324 2d100000 0800e003 440083ac`. Note the store must be scheduled into
  the jr delay slot with v0 zeroed BEFORE the branch; the two-argument variant
  `int f(void* p, int x){ ((int*)p)[0x11]=x; return 0; }` instead emits
  `sw; j $31; move $2,$0` (store first), so pass -1 as a compile-time constant
  in the body, not as a parameter.
- Verification (mechanical):
  - `build/code/game/pause.o` symtab: `func_0021A318` at `.text+0x1608`,
    st_size = 0x10.
  - Object bytes == original slice `ffff03242d1000000800e003440083ac`.
  - Neighbours contiguous: func_0021A310 ends at 0x1608, func_0021A328 starts
    at `.text+0x1618` (unchanged blob).
  - pause.o `.text` total = `0xEA30` == subsegment length (unchanged).
  - `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
