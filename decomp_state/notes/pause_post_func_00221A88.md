# pause_post func_00221A88 (0x221A88, 0x30 bytes)

Matched 2026-09-10 as `pause_releaseStateSoundSlot`. A pause-menu sound-slot
release callback: it takes the state object, hands its sound-slot field
(offset 0x54) to `pause_releaseSoundSlot`, stores the result back, and returns
0.

```c
typedef struct {
    u8 pad[0x54];
    u32 f54;
} PauseSoundSlotState;

int pause_releaseStateSoundSlot(PauseSoundSlotState* state) {
    state->f54 = pause_releaseSoundSlot(state->f54);
    return 0;
}
```

## Original assembly

```
27bdffe0  addiu  sp,sp,-32
7fbf0010  sq     ra,16(sp)
7fb00000  sq     s0,0(sp)
0080802d  move   s0,a0
0c089736  jal    func_00225CD8          (pause_releaseSoundSlot)
8e040054    lw   a0,0x54(s0)            (delay slot: load the slot)
ae020054  sw     v0,0x54(s0)            (store the released slot back)
7bbf0010  lq     ra,16(sp)
0000102d  move   v0,zero
7bb00000  lq     s0,0(sp)
03e00008  jr     ra
27bd0020    addiu sp,sp,32              (delay slot)
```

## Evidence

- Callee `func_00225CD8` is the still-assembly `pause_releaseSoundSlot`: it
  releases a pause sound slot and stops its active music state when necessary.
  Its `asm("func_00225CD8")` declaration sits immediately above this function
  (moved up from lower in the file so it is declared before use).
- Callback group: 0x221A48 / 0x221A88 / 0x221AB8 / 0x221B50 are registered
  together as a four-entry function-pointer table in data at 0x1D265C
  (entries at 0x1D2650..0x1D265C). 0x221A48 is the acquire counterpart (calls
  func_00225C18 and also writes the slot to 0x54 plus a 0x84 field of a global
  object at D_001D5BF4); 0x221AB8 and 0x221B50 are the remaining (still
  INCLUDE_ASM) callbacks of the same menu state.
- Twin: the matched `pause_updateCallback` (0x222F58) is byte-identical in
  shape — `slot = pause_releaseSoundSlot(slot); return 0;` — but operates on a
  different menu-state object whose sound slot is at offset 0x48
  (`PauseCallback {int pad[18]; int f48;}`). That object is the save-data
  menu family (its callback table is at 0x1D17EC/0x1D183C/0x1D4D74/0x1D4F54,
  alongside SavingDataMenu/LoadingDataMenu at 0x2239E0). The two are different
  object types (slot at 0x48 vs 0x54), hence the separate
  `PauseSoundSlotState` struct.

## Mechanism

The C function is placed in the source at the exact spot the INCLUDE_ASM
occupied (between `func_00221A48` and `func_00221AB8`), because the root
linker script packs every object's `.text` contiguously in source order with no
per-symbol addresses. Moving the definition anywhere else shifts the whole
layout (a first attempt that placed it after `pause_updateCallback` produced a
4698-byte ELF diff). EGC emitted the body byte-for-byte on the first probe
(48/48), including the `lw a0,0x54(s0)` in the `jal` delay slot and the
`move v0,zero` between the two `lq`s — no flag or source reorder was needed.

## Symbol / placement

- The C definition emits the cfront-mangled
  `pause_releaseStateSoundSlot__FP19PauseSoundSlotState`; the function lands at
  0x221A88 purely by contiguous `.text` object layout.
- `config/linker_aliases.ld` carries `func_00221A88 = 0x00221A88;` so the
  address-based symbol name still resolves (same pattern as the matched
  `moby_getSecondaryObject` / func_00214228).

## Verification

- `decomp_probe.py` candidate vs
  `code/_generated/nonmatchings/game/pause_post/func_00221A88.s`: 48/48 bytes,
  0 differences.
- Built `0x221A88` objdump matches the original word-for-word:
  `27bdffe0 7fbf0010 7fb00000 0080802d 0c089736 8e040054 ae020054 7bbf0010
   0000102d 7bb00000 03e00008 27bd0020`.
- Full clean build + `cmp build/boot_elf.elf assets/boot_elf.elf` passes;
  count 727 -> 726.
