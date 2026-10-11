# menu_post_createSavePending (was func_00208D20, vram 0x208D20, file 0xA8D20, 64 bytes) — MATCHED 2026-10-11

Source: `code/game/menu_post_mid.cpp` (TU flag `-fno-schedule-insns`, normal
address splitting, SN assembler). C++ free function `void
menu_post_createSavePending(void)` (symbol `menu_post_createSavePending__Fv`,
pinned in config/symbols.txt).

## Semantics

Menu-post callback 14 of the fp table at 0x1A0438 (dispatcher
`menu_dispatchCallback` func_00208840, menu_callbacks.cpp, still blocked).
Reached from the boot flag-resolve callback (slot 13, `func_00208CA8`) when
`menuPostFlags & 0x10`, and from the page-open callback (slot 8,
`func_00208B28`) when there is no error. Body:

```cpp
register int state asm("$4") = MEMCARD_STAGE_IDLE;          // 2
if (g_memcard.stage == state && g_memcard.nextstage < 0) {
    int idx = MENU_POST_CREATING_SAVE_CALLBACK;             // 15
    g_memcard.nextstage = MEMCARD_REQUEST_CREATE_SAVE;      // 9
    g_memcard.nextslot = 0;
    menuPostCallbackIndexGp = idx;                          // continue at slot 15
}
```

- This is the RC1 equivalent of the Deadlocked reference
  `csf_create_save_pending` (reference/dl/game_dl/mcstate.cpp): it arms a
  **create-save** request. When the memcard machine is idle
  (`stage == MEMCARD_STAGE_IDLE`, 2) and no operation is pending
  (`nextstage < 0`), it sets `nextstage = 9` and `nextslot = 0` and continues
  the chain at the creating-save callback (slot 15).
- **Stage 9 = create-save.** It is a one-shot transition in `memcard_Update`
  (0x2093D8): `stage = 10`, `substage = 0`, `card.changed = 0`, fall through
  to stage 10. Stage 10 is the create-save operation: it checks free space
  (`free + corrupt_size < 400` -> err 7), `sceMcMkdir`s the save dir, opens
  the save files for write (`sceMcOpen(..., 0x203)`), and streams the game
  data to them. The reference `csf_create_save_pending` sets the same
  `nextstage = 9` / `nextslot = 0` and transitions to `CS_CREATING_SAVE`.
- **Slot 15 = creating-save** (`func_00208D60`, still INCLUDE_ASM). It polls
  the create-save result: on success it arms the follow-up write
  (`nextstage = 16`/0x10, a second write to the 0x14EED0/0x1506D0 buffers),
  clears `useslot`/`card.changed`, and continues at slot 16; on error it
  routes the chain to the error callback (slot 18, 0x12) and sets
  `menuPostFlags |= 0x40` (MENU_POST_PAGE_OPEN_BLOCKED_FLAG).

## Codegen

Byte-identical clone of the matched `menu_post_pageAdvance` (slot 10,
0x208BC0) with two different constants: `nextstage = 9` (vs 7) and the
callback index `15` (vs 11). The same two load-bearing forms under the TU's
`-fno-schedule-insns` apply unchanged — see
notes/menu_post_mid_menu_post_pageAdvance.md:

1. The state constant pinned to a0 (`register int state asm("$4") = 2;`).
2. The slot-15 constant as a local `int idx = 15;` (forces early v1
   materialization and the DC-before-E0 store order).

## Exact original (16 words, on-disk byte order)

```
1400023C  lui   v0, 0x14
02000424  li    a0, 2
90D24524  addiu a1, v0, %lo(0x13D290)
D400A38C  lw    v1, 0xD4(a1)        # stage
09006414  bne   v1, a0, end
00000000  nop
DC00A28C  lw    v0, 0xDC(a1)        # nextstage
06004104  bgez  v0, end
09000224  li    v0, 9               # bgez delay slot (MEMCARD_REQUEST_CREATE_SAVE)
0F000324  li    v1, 15              # MENU_POST_CREATING_SAVE_CALLBACK
DC00A2AC  sw    v0, 0xDC(a1)        # nextstage = 9
E000A0AC  sw    zero, 0xE0(a1)      # nextslot = 0
1600013C  lui   at, 0x16
B0EE23AC  sw    v1, -0x1170(at)     # menuPostCallbackIndex = 15
0800E003  jr    ra
00000000  nop
```

## Verification

- `make split && make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf`:
  byte-identical. `python3 tools/decomp_status.py --count`: 530 -> 529.
- objdump of both ELFs at 0x208D20 is identical (the `li v0,9` / `li v1,15`
  pair confirms the two constants).
- The stale nonmatching `func_00208D20.s` was removed; the C function is
  compiled from source at the same position (intra-TU source-order placement
  keeps it at 0x208D20).

## Naming

- `MEMCARD_REQUEST_CREATE_SAVE` (9) — the create-save request (stage 9).
- `MENU_POST_CREATING_SAVE_CALLBACK` (15) — the slot-15 creating-save
  callback the chain continues at.
- Function name `menu_post_createSavePending` mirrors the reference
  `csf_create_save_pending`.

Follow-up (not this target): the slot-15 handler `func_00208D60` and the
stage-16 (0x10) write it arms are still INCLUDE_ASM.
