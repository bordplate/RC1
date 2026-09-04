# func_002223D8 (vram 0x2223D8, file 0x123358, 20 bytes) — MATCHED 2026-09-04

Pause-menu init callback. Zeros three int fields of the menu state struct
(`+0x3c`, `+0x40`, `+0x50`) and returns 0:

```
move v0,zero        ; return 0, hoisted above the stores
sw   zero, 60(a0)   ; +0x3c
sw   zero, 64(a0)   ; +0x40
jr   ra
sw   zero, 80(a0)   ; +0x50 (delay slot)
```

## Identity / context

- vtable at vram 0x001D2B18 = { 0x2223F0 (update), 0x222768 (draw),
  0x2223D8 (init) }; a 4th slot exists in sibling tables
  (0x001D2B70/0x001D2BD8 = { 0x21FDC8, 0x220348 DrawCheckingMemoryCardDataMenu,
  0x21FCE0 init, 0x21FD78 destroy }), so the menu callback layout is
  { update, draw, init, destroy? }.
- Pause menu registry: vram 0x001D29EC..0x001D2A04 =
  { 0x25, &tbl_B18, &tbl_AC0, &tbl_B18, &tbl_B70, &tbl_BD8 }, duplicated at
  0x001D2A74..0x001D2A88. Tables: AC0 = {0x219D80,0x21A328,0x21CAE0},
  B18 = this menu, B70/BD8 = memory-card-check menu pair.
- update (0x2223F0): text-list state machine over state +0x50 (cases 0..0x13);
  sets string id D_001D2AF4 (e.g. 0x50A9, 0x50D4, 0x50D6, 0x5106, 0x5136,
  0x5143, 0x5144, 0x5175) and string-list pointer +0x34 (entries like
  &D_001D5098 = { 0x50AA, ... }, i.e. ids into the pause-screen string asset,
  not in-ELF strings); timer +0x3c, counter +0x40, flag +0x54.
- draw (0x222768): renders the string list (+0x34) with scroll/position
  shorts at +0x18..+0x24; calls func_001FDD10/func_001F7580/func_00233980.
- Only xref: the DATA slot 0x001D2B20 in the vtable (no jal anywhere).
- The registry is not accessed gp-relative from text (no 0x1D2xxx gp loads);
  the code reaches it indirectly — not fully traced (not needed for matching).

## Codegen findings

- EGC -G8 -O2: with three independent constant zero-stores sharing a
  REGISTER base (a0), the same fixed permutation as the documented
  %hi/%lo-global-base case applies: emission = [stmt3, stmt1, jr $ra,
  ds:stmt2] and the return-0 `daddu v0,$0,$0` is hoisted above the stores.
  Source order `f40, f50, f3c` -> original `0x3c, 0x40, jr, ds:0x50`.
  Natural order `f3c, f40, f50` gave `0x50, 0x3c, jr, ds:0x40` (wrong).
- `return 0;` idiom reconfirmed: `daddu v0,$0,$0` (matches matched
  func_002217F8, which is just `jr ra` + ds `daddu v0,$0,$0`).
- The `nonmatching func_002223D8, 0xC` size header in the generated .s is
  stale/wrong (actual glabel..endlabel body is 5 words = 0x14; the file also
  carries one trailing nop = 0x18 in file). Use glabel..endlabel contents,
  not the size field, when picking candidates from a scan.

## Struct

Declared locally in pause.cpp (project convention, cf. StreamState in
stream.cpp):

```c
typedef struct {
    u8 pad[0x3C];
    u32 field_3c;
    u32 field_40;
    u8 pad_44[0xC];
    u32 field_50;
} PauseMenuState;
```

Final ELF is stripped (no .symtab in assets/boot_elf.elf), so the C symbol
name does not affect the binary; kept `func_002223D8` per project
naming convention.

## Verification

- Standalone EGC test object (5 words) byte-identical to original:
  `2d100000 3c0080ac 400080ac 0800e003 500080ac` (BE comment form).
- nm build/code/game/pause.o: `func_002223D8` plain T at .text+0x96c8, no
  .NON_MATCHING alias.
- Built ELF slice file 0x123358..0x12336C == original.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- decomp_status --count: 832 -> 831.
