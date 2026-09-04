# func_0021CAC8 (code/game/pause.cpp)

- Original: 20 bytes (`0x14`) at file offset `0x11DA48` in `assets/boot_elf.elf`,
  vram `0x0021CAC8`. Object-relative `.text+0x3db8` in `pause.o`.
- Original instruction sequence (LE words):
  ```
  addiu   $v0, $zero, -0x1         # ff ff 02 24
  lui     $v1, %hi(D_001A0318)     # 1a 00 03 3c   (hi = 0x001a)
  sw      $v0, %lo(D_001A0318)($v1)# 18 03 62 ac   (lo = 0x0318)
  jr      $ra                      # 08 00 e0 03
    move    $v0, $zero             # 2d 10 00 00 (delay slot, returns 0)
  ```
  Note: the Splat-generated `.s` renders the last word as `daddu $2,$0,$0`, but
  word `0000102d` is actually `move $v0,$zero` (objdump agrees). Both set v0=0.
- Semantics: sets the global `D_001A0318 = -1` and returns 0. A pause-menu
  "reset" callback: Ghidra shows a single DATA xref at `0x001d2458` (an entry in
  a pause-menu item descriptor table, same family as the matched func_0021A308/
  A310/A318/BD98/D1F0 callbacks). The target global `D_001A0318` (Ghidra vram
  `0x001A0318`) is READ by many menu functions (`0x0020547c`, `0x002054c4`, ...
  across `0x00205xxx`-`0x00206xxx`) and WRITTEN by `0x0021c250` and by this
  function — i.e. a shared pause-menu state/index word that this callback
  reinitialises to the sentinel `-1`.
- Global placement: `D_001A0318` is a `D` symbol in
  `build/code/_generated/build/data/data.data.o` (offset `0x3ae98`), linked into
  the `.data` segment (vram `0x165480`-`0x1e8b77`). Its link address is ~230KB
  from `$gp` (`0x166c00`, set by `--defsym _gp=0x166c00`), well outside the 32KB
  small-data window, so it MUST be declared with the section attribute to force
  absolute (lui) addressing:
  ```cpp
  extern "C" int D_001A0318 __attribute__((section(".data")));
  ```
  Without the attribute, EGC `-G8` emits gp-relative `sw $v1,0($gp)` (a single
  store that lands in the jr delay slot) — wrong structure/size for this global.
  This is the same "outside-gp-window => section(\".data\")" rule as the matched
  core.data globals (menu.cpp u8 flags) and music.cpp `D_001516D0`.
- Replacement (at the old INCLUDE_ASM site, code/game/pause.cpp):
  ```cpp
  extern "C" int D_001A0318 __attribute__((section(".data")));

  extern "C" int func_0021CAC8(void) {
      D_001A0318 = -1;
      return 0;
  }
  ```
- EGC 2.95.2 codegen (`-G8 -O2 -ffast-math -fno-exceptions`): with the section
  attribute, standalone and in-tree compiles both emit exactly
  `addiu $v0,$0,-1; lui $v1,%hi; sw $v0,%lo($v1); jr $ra; <move $v0,$0>` — the
  -1 value lives in v0, the base in v1, the store is a real instruction (lui
  must precede it, so it cannot take the delay slot), and the `return 0`
  (`move $v0,$zero`) fills the jr delay slot.
- Verification (mechanical):
  - `build/code/game/pause.o` symtab: `func_0021CAC8` plain `T` at `.text+0x3db8`,
    st_size `0x14`; no `.NON_MATCHING` alias.
  - Only relocations in `[0x3db8,0x3dcb)` are `R_MIPS_HI16@0x3dbc` +
    `R_MIPS_LO16@0x3dc0`, both against `D_001A0318`.
  - Object bytes with relocs resolved (hi=`0x001a`, lo=`0x0318`) == original
    slice: `2402ffff 3c03001a ac620318 03e00008 0000102d`.
  - Contiguous: `func_0021CA98` (`.text+0x3d88`) ends at `0x3db8`,
    `func_0021CAE0` (`.text+0x3dd0`) follows.
  - `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
  - `decomp_status`: active `832` -> `831`.
