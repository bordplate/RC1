# Hud_ResetChannelBySerial (func_001FF480, code/game/hud_post_resetchan.cpp) — matched 2026-10-09

## Semantics

Resets one HUD channel slot by serial. Scans the 13 `hudChanSlots` for a slot
whose `serial` field (`HudChanSlot.serial`, +0x64) equals the argument; on a
match it requests `Hud_SetChannelPending(i, HUD_SLOT_RESET_ICON_ID, 0, 0, 0, 0,
0)` (reset id, no callback) and returns 1, else returns 0. `0xFFFF` is the
empty/reset icon id — `Hud_InitBanks` assigns it to every slot at startup.

Both callers (0x216D00, 0x216E78) live in `func_00216C48`
(`code/game/stream.cpp`), an `INCLUDE_ASM` that branches to the raw label
`func_001FF480`; the `asm("func_001FF480")` override keeps linkage.

## The SN-vs-GNU `#nop` split (why this function has its own TU)

The matching C form is the plain for-loop:

```cpp
int i;
for (i = 0; i < HUD_SLOT_COUNT; i++) {
    if (hudChanSlots[i].serial == serial) break;
}
if (i < HUD_SLOT_COUNT) { Hud_SetChannelPending(i, HUD_SLOT_RESET_ICON_ID, 0, 0, 0, 0, 0); return 1; }
return 0;
```

EGC peels the first slot out of the loop and schedules the iterator `i++` into
the back-branch delay slot, emitting a scheduler `#nop` immediately before the
back-branch `bnel` at 0x1FF4B8. That one byte is assembler-dependent:

- ps2eeas (SN, `-snas`) expands the `#nop` token into a real `nop` — matching
  the original (128 bytes, 0 diffs).
- gas (GNU, `-Wa,-EL`) treats `#nop` as a comment and drops it — 124 bytes,
  the `bnel` target shifts, beq offset 0x0a->0x09.

`-fno-schedule-insns` / `-fno-schedule-insns2` do not change the GNU output.
The original carries the nop, so it was assembled with SN. The rest of the old
`hud_post.o` (the 0xFF500+ `INCLUDE_ASM` funcs) requires the GNU override, so
the fix is a Splat boundary split giving this function its own SN TU.

## Changes

- `config/RC1.yaml`: `[0xff418, cpp, game/hud_post]` split at 0xff480 and
  0xff500 into `hud_post` / `hud_post_resetchan` / `hud_post_post`.
- `code/game/hud_post_resetchan.cpp` (new, SN TU): the C function above.
- `code/game/hud_post.cpp` (GNU TU): trimmed to `Hud_CommitChannel`.
- `code/game/hud_post_post.cpp` (new, GNU TU): the 0xFF500+ `INCLUDE_ASM`s and
  `hud_updateMessageTimer`.
- `Makefile`: `game/hud_post_post.o` added to the GNU `ASSEMBLER_FLAGS` list;
  `hud_post_resetchan.o` stays on the default SN assembler.
- `code/include/hud.h`: `#define HUD_SLOT_COUNT 13` and
  `#define HUD_SLOT_RESET_ICON_ID 0xFFFF` (shared constants); the duplicate
  local `HUD_SLOT_COUNT` removed from `code/game/hud_pre.cpp`.

## Verification

- `tools/decomp_probe.py` (SN): 128 bytes, 0 word diffs vs the reference.
- `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf`: identical.
- `tu_assembler_diff.py`: `func_001FF480 128 0x1ff480 match`.
- objdump: 0x80-byte function at 0x1FF480, scheduler nop at 0x1FF4B8, `bnel`
  at 0x1FF4BC, `jal Hud_SetChannelPending__Fiiiiiii` at 0x1FF4E8.
- Linker order: hud_post.o, hud_post_resetchan.o, hud_post_post.o.
- `tools/decomp_status.py --count`: 552 -> 551.
