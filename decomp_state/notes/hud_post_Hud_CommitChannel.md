# Hud_CommitChannel (0x1FF418, was func_001FF418)

Commits a pending HUD channel request. ELF symbol is the Splat placeholder
`func_001FF418` (boot ELF is stripped, so the linker script pins that label);
the C name is bound to it with an asm override (declared in code/include/hud.h).
104 bytes (0x68).

## What it does

Given a `HudChanSlot*` (a 0x90-byte channel-slot record in `hudChanSlots`):
1. `func_001FF500(slot, slot->pendId)` — sets up the slot's icon fields
   (0x00, 0x40, 0x42, 0x44) from the icon-table entry for `pendId`
   (callee still an INCLUDE_ASM placeholder in hud_post.cpp).
2. Copies the pending block into the active block:
   `mode=pendMode, d=pendD, e=pendE, c=pendC, b=pendB`.
3. `slot->fn = slot->pendFn` — **unconditional** (see below).
4. If `pendFn != 0`: invoke the committed callback `slot->fn(slot)`.
5. `slot->pending = 0`.

Caller: `Hud_SetChannelPending` (0x1FF308, matched) calls
`Hud_CommitChannel(slot)` when `mode & slot->mode & 0x20` holds.

## The match-sensitive part

The `slot->fn = slot->pendFn` store is **unconditional** — it is NOT inside
the `if`. In the original it lands in the `beqz` delay slot:

```
beqz   $7, .L            # if (pendFn == 0) skip the call
sw     $7, 0x10($16)     # slot->fn = pendFn  (delay slot: ALWAYS runs)
jalr   $7                #   slot->fn(slot)
daddu  $4, $16, $0       #   a0 = slot (delay slot)
.L:
sw     $0, 0x68($16)     # slot->pending = 0
```

So the store runs on both edges; only the `jalr` is conditional. Writing the
store inside the `if` (the natural reading) fails two ways: EGC then loads
`pendFn` last (after the five copy loads) instead of first, and it moves
`sw b` into the `beqz` delay slot (displacing the branch). Making the store
unconditional restores the original load order (pendFn first) and the
delay-slot placement, giving a 0-diff match.

The five copies (`mode/d/e/c/b`) load into v0/v1/a0/a1/a2 and store in the
same order; `pendId` loads in the `jal func_001FF500` delay slot; `slot`
stays in s0. No pins, no flags, no volatile — default SN codegen.

## Verification (mechanical)

- Probe (working/hud_post_func_001FF418/probe, SN, project flags): 104/104
  bytes, 0 diffs.
- `tu_assembler_diff build/code/game/hud_post.o build/boot_elf.elf`: 28/28 match.
- `tu_assembler_diff build/code/game/hud_chan.o build/boot_elf.elf`: 1/1 match.
- Clean `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf`: byte-identical.

## Cross-file changes (same coherent change)

- code/include/hud.h: added `Hud_CommitChannel` declaration (asm override to
  `func_001FF418`); updated the HudChanSlot doc comment.
- code/game/hud_post.cpp: replaced `INCLUDE_ASM(..., func_001FF418)` with the
  C definition at the same source position (intra-TU addresses preserved);
  added a forward declaration for the callee `func_001FF500`.
- code/game/hud_chan.cpp: the caller now uses `Hud_CommitChannel` (its local
  `asm("func_001FF418")` declaration removed in favor of the hud.h one).

`func_001FF500` (the icon-setup callee) remains an INCLUDE_ASM placeholder and
is a separate queue target.
