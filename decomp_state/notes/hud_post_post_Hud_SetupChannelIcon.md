# Hud_SetupChannelIcon (0x1FF500, was func_001FF500)

Sets up a channel slot's committed-icon fields from the icon-table entry for
`iconId`. 104 bytes (0x68). ELF symbol is the Splat placeholder `func_001FF500`;
the C name is bound to it with an asm override declared in code/include/hud.h.

## What it does

```
index = Hud_GetIconIndex(iconId);
slot->iconId       = iconTable[index].id;        // +0x00 (sw, u16 zero-ext)
slot->iconIndex    = index;                      // +0x40 (sh)
slot->iconAnimType = iconTable[index].animType;  // +0x42 (sb)
slot->iconStart    = iconTable[index].start;     // +0x44 (sw, u16 zero-ext)
```

`iconTable` is `hudHeap.iconTable` (u32 at +0x1C of hudHeap, 0x19A404) pointing
at the `HudIconDef` array (8-byte entries, DL `icon_t`), the same table
`Hud_GetIconIndex` scans. Sole caller: `Hud_CommitChannel` (0x1FF418, matched)
calls it as `Hud_SetupChannelIcon(slot, slot->pendId)`.

## The match-sensitive part

- The `hudHeap.iconTable` value is loaded fresh before EACH field access
  (three `lw r,0x1C(a2)` sequences); only the `&hudHeap` base (a2) is hoisted
  once. Writing the table base into a local pointer CSEs the loads to one and
  fails; re-deriving `((HudIconDef*)hudHeap.iconTable)[index]` per statement
  keeps the three reloads (EGC does not CSE a global whose value could be
  clobbered by the intervening stores through the `slot` pointer).
- Statement order is `iconId, iconIndex, animType, start`. With iconIndex
  first, EGC hoists the `sh` to right after the `jal` (then `sll v0,v0,3`
  overwrites the raw index, &hudHeap lands in a1, and every downstream field
  load uses a0: ~18 word diffs). With iconId first, EGC keeps the raw index
  in v0 live across the id load for the later `sh`, computes `sll a0,v0,3`,
  puts &hudHeap in a2, loads the id into a1, and stores `sh; sw` adjacent —
  byte-identical.
- No pins, no flags, no volatile: default SN codegen.

## Cross-file changes (same coherent change)

- code/include/hud.h: `HudIconDef` moved here from hud_icon.cpp (the shared
  icon-table entry layout); `Hud_GetIconIndex` and `Hud_SetupChannelIcon`
  declarations added (asm override for the latter); `HudChanSlot` field_00
  renamed iconId and pad_3C split into iconIndex/iconAnimType/pad_43/
  iconStart (0x40-0x47).
- code/game/hud_icon.cpp: local HudIconDef typedef removed (now hud.h);
  Hud_GetIconIndex codegen unchanged (1/1 tu_assembler_diff).
- code/game/hud_post_post.cpp: INCLUDE_ASM replaced by the C definition at the
  same source position (intra-TU addresses preserved).
- code/game/hud_post.cpp: local `func_001FF500` forward declaration removed;
  the caller now uses `Hud_SetupChannelIcon` from hud.h (same asm label, same
  signature — codegen unchanged, 1/1).

## Verification (mechanical)

- Probe (working/ff500/probe2, SN, project flags): 104/104 bytes, 0 diffs.
- `tu_assembler_diff build/code/game/hud_post_post.o build/boot_elf.elf`:
  26/26 match.
- hud_post / hud_icon / hud_chan / hud_pre objects: all match after the
  shared-header change.
- Clean `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf`: byte-identical.
