# func_002069B0 (code/game/menu.cpp)

- Original: 12 declared bytes at file offset `0x107930` in
  `assets/boot_elf.elf`, vram `0x002069B0` (menu subsegment starts at file
  `0x1078f8`; function sits at object-relative `.text+0x38`).
- Original instruction sequence (the final word is the `jr $ra` delay slot,
  owned by this function even though splat's declared size is 0xC):
  ```
  lui     $v1, %hi(D_0013D395)   # 1400033c
  lbu     $v0, %lo(D_0013D395)($v1)  # 95d36290
  jr      $ra                    # 0800e003
  sltu    $v0, $zero, $v0        # 2b100200  (delay slot)
  ```
  i.e. `return D_0013D395 != 0;` for the unsigned byte at core.data vram
  `0x0013D395`.
- Identity/semantics: part of a family of byte-flag predicates in the menu
  code (siblings func_002069A0/D_0013D394, func_002069C0/D_0013D39D,
  func_00206B88-BB8-BC8/D_0013D3A4-A7-AD all share the identical
  `lui; lbu; jr $ra; sltu` shape). Ghidra shows no function boundary here
  (folded region); a DATA xref pattern similar to the func_00207A08/10 notes
  places this family among predicate-style callbacks. No code in the boot ELF
  writes D_0013D395 directly (only read xref at 0x002069B4), so it is likely
  set via indexed stores or by overlay code; exact consumer not pinned down,
  hence the name kept as `func_002069B0`.
- The global is defined as a dlabel in
  `code/_generated/build/data/core.data.data.s` (section `.data`) and also
  aliased at `0x13D395` in `build/undefined_syms_auto.txt`, so an extern from C
  binds correctly (same pattern as `D_152078` in framebuf.cpp).
- Replacement (at the old INCLUDE_ASM site, code/game/menu.cpp:7):
  ```cpp
  extern u8 D_0013D395 __attribute__((section(".data")));

  extern "C" int func_002069B0(void) {
      return D_0013D395 != 0;
  }
  ```
  (`#include "types.h"` added to menu.cpp for the `u8` typedef, matching
  framebuf.cpp.)
- EGC 2.95.2 codegen check (standalone `-S` test with
  `-G8 -O2 -ffast-math -fno-exceptions`): emits exactly
  `lui $3,%hi(sym); lbu $2,%lo(sym)($3); j $31; sltu $2,$0,$2` - same registers
  as the original.
- Verification (mechanical):
  - `readelf -s build/code/game/menu.o`: `func_002069B0` at `.text+0x38`,
    st_size=0x10 (the delay-slot word is part of the compiled range);
    no `.NON_MATCHING` alias symbol remains.
  - Raw object bytes at `.text+0x38` carry R_MIPS_HI16/LO16 placeholders for
    D_0013D395, so object-slice comparison is invalid here (see learned caveat
    in func_00207A10.md); linked-output comparison used instead.
  - `build/boot_elf.elf` slice at file offset `0x107930..0x107940`:
    `1400033c95d362900800e0032b100200` == original slice.
  - Neighbours contiguous exactly as original: func_002069A0 at `.text+0x28`,
    func_002069C0 at `.text+0x48`.
  - menu.o `.text` total = `0x26b8` == subsegment length (unchanged).
  - `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
