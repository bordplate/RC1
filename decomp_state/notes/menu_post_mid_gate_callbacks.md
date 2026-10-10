# menu_post_gate{PendingSelection,PageOpen,BootSelection} (0x208A38 / 0x208B88 / 0x208C70) — MATCHED 2026-10-10

Three menu-post callback-table functions in `menu_post_mid.cpp`
(`-fno-schedule-insns`, normal address splitting, SN assembler). All three
share one shape:

```cpp
if (menu_postHasPendingSelection != C) {   // C = -2 (sentinel) or 0
    menuPostCallbackIndexGp = MENU_POST_SELECT_NEXT_PAGE_CALLBACK; // 3
    return;
}
if (menuPostFlagsGp & M)                   // M = 0x2 (A38, C70), 0x6 (B88)
    menuPostCallbackIndexGp = K;           // K = 6 (A38), 10 (B88), 13 (C70)
```

They sit in the post-callback state machine at table 0x1A0438 (dispatcher
func_00208840, still INCLUDE_ASM): slots [5], [9], [12] respectively,
falling through to the still-unmatched slots 6/10/13 handlers
(func_00208A78/00208BC0/00208CA8).

## Key codegen finding: plain in-window alias replaces the constant cast

The 0x15EEB0 store appears in TWO forms across this family:

1. `sw v0,-0x7d50($28)` (GPREL16) when it lands in the epilogue `jr $ra`
   noreorder delay slot (the three gate functions, func_00208AF8, and the
   0x208A78-style handlers).
2. `lui at,0x16; sw v?,...($at)` (absolute, at-base) when it sits in the
   body outside a noreorder block (func_002089A8, func_00208EB8).

GNU-assembler-era notes (menu_func_00208EB8.md, 2026-09-04/11) concluded the
absolute form REQUIRES a constant-address cast, because gas expands the bare
small-data pseudo GPREL16 from the end-of-file `.extern`. Under the current
SN pipeline that is wrong: ps2eeas expands an UNSEEDED bare pseudo in place
to the absolute self-based `lui at; sw` pair (single-pass, no end-of-file
`.extern` for it), while inside a `.set noreorder` region the same bare
pseudo stays a GPREL16. So ONE plain declaration

```cpp
extern int menuPostCallbackIndexGp;   // alias in config/linker_aliases.ld
                                      // at 0x15EEB0 (same as menuPostCallbackIndex)
```

produces BOTH forms correctly, and the constant cast (and the
`MENU_POST_CALLBACK_INDEX_ADDRESS` #define) were removed from
menu_post_mid.cpp / menu_post_gadgets.cpp / menu.h on 2026-10-10. This also
clears the two 2026-09-16 policy violations that AGENTS.md flagged for
func_002089A8/func_00208EB8 — the plain form matches, so no
revert-and-block was needed.

`menuPostFlagsGp` (0x15EEB4) is the same plain alias for the flag loads that
sit in branch delay slots (GPREL form). The `.data`-section declaration of
`menuPostCallbackIndex` in menu.h must NOT be used for the absolute stores:
it compiles to a two-register split load with a general-register base, not
the at-base pair.

`menu_postHasPendingSelection` (0x13D2AC = menuStateData+0x1C, `.data`) loads
self-based outside noreorder; the struct-field spelling `menuStateData.selected`
is what the siblings (func_00208AF8) use — both spellings stay in the
codebase because each compiles to the register form its function needs.

## 0x208A38 register pin

The original materializes the -2 sentinel in `a0` (`addiu $4,$0,-0x2`) and
the loaded selected value in `v1`, comparing `bne $3,$4`. Without a pin EGC
puts the constant in `v1` and the load in `a0` (one-word swap at the
compare). `register int sentinel asm("$4") = -2;` preserves the split. The
two siblings (compare `beqz v1`) need no pin.

## Source

- `code/game/menu_post_mid.cpp`: three INCLUDE_ASM placeholders replaced in
  place (intra-TU addresses follow source order); alias externs at top.
- `code/game/menu_post_gadgets.cpp`: func_00208EB8 cast replaced by the
  plain alias store.
- `code/include/menu.h`: `MENU_POST_CALLBACK_INDEX_ADDRESS` removed;
  constants added: MENU_POST_FLAG_02 (0x2), MENU_POST_FLAG_04 (0x4),
  MENU_POST_SELECTION_NONE (-2), MENU_POST_FLAG_RESOLVE_CALLBACK (6),
  MENU_POST_PAGE_ADVANCE_CALLBACK (10),
  MENU_POST_BOOT_FLAG_RESOLVE_CALLBACK (13).
- `config/linker_aliases.ld`: `menuPostFlagsGp = 0x0015EEB4;` added
  (menuPostCallbackIndexGp already present).
- `config/symbols.txt`: mangled names for the three functions.

## Verification

- `tools/decomp_probe.py` per function: match true (flags
  `-fno-schedule-insns`, defines for the two aliases).
- `tools/tu_assembler_diff.py build/code/game/menu_post_mid.o`:
  14/14 match; `menu_post_gadgets.o`: 1/1 match.
- `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf` passes. decomp_status --count 540 -> 537.

## Naming

The three fall-back table slots (6/10/13) are still INCLUDE_ASM, so the
"resolve/advance/boot" suffixes describe the dispatch target rather than a
fully established behavior; the gating on the pending selection is
byte-evidenced. Flag bits 0x2/0x4 remain unestablished
(MENU_POST_FLAG_02/04 placeholders).
