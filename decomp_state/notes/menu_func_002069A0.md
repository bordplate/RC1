# func_002069A0 (code/game/menu.cpp)

- Original: 12 declared bytes at file offset `0x107920` in
  `assets/boot_elf.elf`, vram `0x002069A0` (menu subsegment base vram
  `0x206978`; function at object-relative `.text+0x28`).
- Original instruction sequence (the final word is the `jr $ra` delay slot,
  owned by this function even though splat's declared size is 0xC):
  ```
  lui     $v1, %hi(D_0013D394)   # 1400033c
  lbu     $v0, %lo(D_0013D394)($v1)  # 94d36290
  jr      $ra                    # 0800e003
  sltu    $v0, $zero, $v0        # 2b100200  (delay slot)
  ```
  i.e. `return D_0013D394 != 0;` for the unsigned byte at core.data vram
  `0x0013D394`.
- Identity/semantics: member of the menu byte-flag getter family documented
  in menu_func_002069B0.md — sits immediately before func_002069B0
  (D_0013D395) and func_002069C0 (D_0013D39D), all sharing the identical
  `lui; lbu; jr $ra; sltu` shape. Ghidra shows no function boundary here
  (folded region); its only xref is the read at 0x2069A4 by itself, and no
  boot-ELF code writes D_0013D394 directly, so it is likely set via indexed
  stores or overlay code. Name kept as `func_002069A0`.
- The global is defined as a dlabel in
  `code/_generated/build/data/core.data.data.s` (section `.data`) and also
  aliased at `0x13D394` in `build/undefined_syms_auto.txt`, so an extern from
  C binds correctly (same pattern as D_0013D395).
- Replacement (at the old INCLUDE_ASM site, code/game/menu.cpp:6):
  ```cpp
  extern u8 D_0013D394 __attribute__((section(".data")));

  extern "C" int func_002069A0(void) {
      return D_0013D394 != 0;
  }
  ```
- Verification (mechanical):
  - `nm -n build/code/game/menu.o`: `func_002069A0` at `.text+0x28`, next
    symbol func_002069B0 at `.text+0x38` -> st_size=0x10; no
    `.NON_MATCHING` alias remains.
  - `build/boot_elf.elf` slice at file offset `0x107920..0x107930`:
    `1400033c94d362900800e0032b100200` == original slice.
  - Neighbours contiguous exactly as original: func_00206978 ends at
    `.text+0x28`, func_002069B0 starts at `.text+0x38`.
  - menu.o `.text` total = `0x26b8` == subsegment length (unchanged).
  - `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes on first
    build.
