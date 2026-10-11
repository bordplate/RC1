# menu_post_pageAdvance (was func_00208BC0, vram 0x208BC0, file 0x109BC0, 64 bytes) — MATCHED 2026-10-11

Source: `code/game/menu_post_mid.cpp` (TU flag `-fno-schedule-insns`, normal
address splitting, SN assembler). C++ free function `void
menu_post_pageAdvance(void)` (symbol `menu_post_pageAdvance__Fv`).

## Semantics

Menu-post callback 10 of the fp table at 0x1A0438 (dispatcher
`menu_dispatchCallback` func_00208840, menu_callbacks.cpp, still blocked).
Queued by `menu_post_gatePageOpen` (slot 9, 0x208B88) when
`menuPostFlags & (MENU_POST_FLAG_02 | MENU_POST_FLAG_04)` and there is no
pending selection. Body:

```cpp
register int state asm("$4") = MENU_POST_PAGE_STATE_2;      // 2
if (menuStateData.field_0xD4 == state && menuStateData.field_0xDC < 0) {
    int idx = MENU_POST_CALLBACK_SLOT_11;                   // 11
    menuStateData.field_0xDC = MENU_POST_PENDING_PAGE_7;    // 7
    menuStateData.field_0xE0 = 0;
    menuPostCallbackIndexGp = idx;                          // continue at slot 11
}
```

- `field_0xD4` (0x13D364) is the menu page state. The value 2 is the state
  tested by BOTH this function (slot 10) and the page-open callback
  `func_00208B28` (slot 8) before either acts; bmain.cpp:166 polls while the
  state is `>= 3`. The full enum (0/1/2/>=3) is not yet decoded.
- `field_0xDC` (0x13D36C) is the pending-page word: -1 = unset
  (MENU_POST_UNSET_PAGE_INDEX, selectNextPage), 3 = resolved
  (MENU_POST_PENDING_PAGE_RESOLVED, preparePageOpen), 7 = written here.
- `field_0xE0` (0x13D370) is zeroed here and by preparePageOpen; -1 by
  selectNextPage.
- When the condition holds the callback index is advanced to slot 11
  (0x208C00, still INCLUDE_ASM); otherwise the index is left unchanged, which
  halts the chain (the dispatcher only continues when the state word changes).

## Codegen findings

Under the TU's `-fno-schedule-insns` two source forms are required for a
byte match:

1. **The state constant must be pinned to a0.** The original materializes the
   `2` in a0 (`li a0,2`) with the menuStateData base in a1
   (`lui v0; addiu a1,v0,lo`) and the first load in v1
   (`lw v1,0xD4(a1)`). Without the pin EGC puts the base in a0 and the
   constant in v0 (7-word diff in the prologue + the two struct stores).
   `register int state asm("$4") = 2;` reproduces the a0/a1/v1 split. This is
   the same constant/value register-split pin as menu_post_gatePendingSelection
   (see notes/menu_post_mid_gate_callbacks.md).

2. **The slot-11 constant must be a local, not a direct store.** Writing
   `menuPostCallbackIndexGp = 11;` directly makes EGC emit the two struct
   stores first (in its fixed zero-store-before-constant-store order:
   `sw zero,0xE0` then `sw v0,0xDC`) and materializes the 11 late (in v0). The
   original instead emits `li v0,7` (bgez delay slot), `li v1,11`, `sw
   v0,0xDC`, `sw zero,0xE0`, `lui at; sw v1` — i.e. the 11 is materialized into
   v1 BEFORE the struct stores and the stores come out DC-then-E0. Introducing
   `int idx = 11;` inside the block (a use of the value at declaration) forces
   the early v1 materialization and the DC-before-E0 store order. A register
   pin (`register int idx asm("$3") = 11;`) also matches but is unnecessary;
   the plain local is what is used.

   Note the tension: with the default scheduler the body (hoisted `li v1,11` +
   DC-before-E0) matches but the prologue register allocation flips (hi->v1,
   load->v0); with `-fno-schedule-insns` the prologue matches and the local-idx
   form recovers the body. Both the pin and the local are therefore load-bearing
   under the TU flag.

## Exact original (16 words, on-disk byte order)

```
3C020014  lui   v0, 0x14
02000424  li    a0, 2
90D24524  addiu a1, v0, %lo(0x13D290)
D400A38C  lw    v1, 0xD4(a1)        # field_0xD4
09006414  bne   v1, a0, end
00000000  nop
DC00A28C  lw    v0, 0xDC(a1)        # field_0xDC
06004104  bgez  v0, end
07000224  li    v0, 7               # bgez delay slot
0B000324  li    v1, 11
DC00A2AC  sw    v0, 0xDC(a1)        # field_0xDC = 7
E000A0AC  sw    zero, 0xE0(a1)      # field_0xE0 = 0
1600013C  lui   at, 0x16
B0EE23AC  sw    v1, -0x1170(at)     # menuPostCallbackIndex = 11
0800E003  jr    ra
00000000  nop
```

## Verification

- `tools/decomp_probe.py` (flags `-fno-schedule-insns`, snas, defines
  menuStateData=0x13D290 menuPostCallbackIndexGp=0x15EEB0): match true, 0/64.
- `tools/tu_assembler_diff.py --all build/code/game/menu_post_mid.o
  build/boot_elf.elf`: 14/14 match.
- `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf`: byte-identical. decomp_status count 532 -> 531.

## Naming / follow-up

Positional constant names (MENU_POST_PAGE_STATE_2, MENU_POST_PENDING_PAGE_7,
MENU_POST_CALLBACK_SLOT_11) and the MenuState field placeholders
(field_0xD4/0xDC/0xE0) are tracked in refactor.json for a semantic decode of
the page-state machine and the slot-11 handler (0x208C00).
