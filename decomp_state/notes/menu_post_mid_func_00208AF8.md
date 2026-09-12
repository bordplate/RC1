# menu_post_preparePageOpen (was func_00208AF8, vram 0x208AF8, file 0x109A78, 44 bytes) — MATCHED 2026-09-12

Source: `code/game/menu_post_mid.cpp` (TU flag `-fno-schedule-insns`, normal
address splitting). C++ free function `void menu_post_preparePageOpen(void)`
(symbol `menu_post_preparePageOpen__Fv`).

## Semantics

Menu-post callback 7 of the fp table at 0x1A0438 (dispatcher
`func_00208840`, menu_callbacks.cpp:21, still blocked). Called when
`menuPostCallbackIndex` == 7, which table[6] (0x208A78) queues after
clearing bit 0x8 of `menuPostFlags`. Body:

```cpp
int pendingPage = menuStateData.field_0xDC;   // 0x13D36C
menuStateData.selected = 0;                   // 0x13D2AC == menu_postHasPendingSelection
if (pendingPage < 0) {
    menuStateData.field_0xE0 = 0;             // 0x13D370
    menuStateData.field_0xDC = MENU_POST_PENDING_PAGE_RESOLVED;  // 3
}
menuPostCallbackIndexGp = MENU_POST_OPEN_PAGE_CALLBACK;          // 8
```

- The `selected = 0` store is UNCONDITIONAL: it sits in the `bgez` delay slot
  (delay slots execute regardless of the branch). A first draft that put the
  store inside the if made EGC hoist the final `li v0,8` into the delay slot
  instead (13 words vs 12).
- `field_0xDC` (0x13D36C) is the pending-page word that
  `menu_post_selectNextPage` resets to -1; writing 3 (>= 0) marks it
  resolved so the next callback, table[8] (0x208B28), skips its page-open
  check (`bgez field_0xDC, end`). Return value is ignored by the dispatcher
  (the 8 left in v0 is residue).
- Context: callback chain for one selection pass is
  [19]0x208EB8 (if selected, queue 3) -> [3]selectNextPage -> [4]0x2089D0
  (dispatch on selected: 0 -> clear 0x20 flag, queue 9; -1 -> selected=0,
  queue 9; -2 -> queue 5) -> [5]0x208A38 / [6]0x208A78 (flag checks, queue 7
  on 0x8 bit) -> [7] this -> [8]0x208B28 (if menuStateData+0xD4 == 2 and
  field_0xDC < 0: if +0xE4 set, queue 17 openInventory + 0x40 flag, else
  re-queue 0). `selected` (0x13D2AC) holds the pending action code
  (restored from `saved` 0x13D34C by table[0] `menu_restoreSelection`);
  -1/-2 are the sentinel actions. The layout of the 0x13D290 block beyond
  these words remains a menu.cpp project item.

## Exact original (12 words, on-disk byte order)

```
1400023C  lui   v0, 0x14
90D24424  addiu a0, v0, %lo(0x13D290)     # menuStateData base in a0
DC00838C  lw    v1, 220(a0)               # field_0xDC in v1
04006104  bgez  v1, +8
1C0080AC  sw    zero, 28(a0)              # selected = 0 (bgez delay slot)
03000224  li    v0, 3
E00080AC  sw    zero, 224(a0)             # field_0xE0 = 0
DC0082AC  sw    v0, 220(a0)               # field_0xDC = 3
08000224  li    v0, 8
0800E003  jr    ra
B08282AF  sw    v0, -0x7D50(gp)           # menuPostCallbackIndex = 8 (jr ds)
00000000  nop
```

## Codegen findings

1. **GP-relative callback-index store needs a plain alias symbol.** The
   store is `sw v0,-0x7D50(gp)` (GPREL16 to 0x166C00-0x7D50 = 0x15EEB0),
   unlike every sibling in this range, which use the at-based absolute form
   (`lui at,0x16; sw v0,-4432(at)`) from the section(".data") declaration or
   the constant-cast idiom. A plain `extern int menuPostCallbackIndex;`
   redeclaration in this TU does NOT strip the section attribute inherited
   from menu.h (probed: EGC keeps emitting the absolute lui/sw pair).
   Fix: new linker alias `menuPostCallbackIndexGp = 0x0015EEB0;` in
   config/linker_aliases.ld (same object as menuPostCallbackIndex), declared
   plain in menu_post_mid.cpp. GPREL16 links fine because 0x15EEB0 is
   in-window.
2. **The condition value must be a local.** With direct
   `if (menuStateData.field_0xDC < 0)` EGC allocates the struct base to v1
   and the loaded value to v0; the original uses base a0 / value v1.
   Loading into `int pendingPage = ...` first flips the allocation to the
   original registers (probed standalone with the TU flags: direct field
   access in the condition = wrong registers, local = match; a reference
   form `MenuState& s = ...` also gave the wrong allocation).
3. The tail `li v0,8; jr ra; sw v0,gp-rel` is the standard independent-store
   into the jr delay slot; no flag or ordering tricks needed beyond (1)+(2).

## Verification

- Built slice 0x109A78..0x109AA4 words (on-disk byte order)
  `1400023C 90D24424 DC00838C 04006104 1C0080AC 03000224 E00080AC DC0082AC
  08000224 0800E003 B08282AF 00000000` byte-identical to the original;
  objdump of build/boot_elf.elf matches assets/boot_elf.elf at 0x208AF8..0x208B24.
- Data table reference renamed: config/symbols.txt now has
  `menu_post_preparePageOpen__Fv = 0x00208AF8;` and the regenerated
  data.data.s emits `.word menu_post_preparePageOpen__Fv` at 0x1A0454
  (same mechanism as menu_restoreSelection).
- Full `make split && make` then `cmp build/boot_elf.elf assets/boot_elf.elf`
  passes; decomp_status count 719 -> 718.
