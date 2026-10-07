# Hud_InitBanks__Fv (code/game/hud_pre.cpp) — matched 2026-10-07

Was `func_001FEE88` (code/game/hud.cpp, first function of the `game/hud`
segment). Renamed; the caller `Transition_DoTransition__Fv` does
`jal Hud_InitBanks__Fv` right before `LoadHudBanks__Fv`.

## Semantics

HUD channel-slot + bank init, called on level transition:

1. Clear the two leading `hudHeap` words: +0x00 (the free-running slot
   serial, incremented and assigned by the slot allocator `func_001FF308`)
   and +0x04 (meaning still unknown; Lombyte's same-build recovery calls it
   `unk4`).
2. Reset the 13 `HudChanSlot` records (0x90 bytes each at `hudChanSlots`,
   0x199B60). The loop keeps a running `int*` anchor at `base + 0x24`
   (`+= 0x90` per record) and writes, relative to the anchor:
   `[0x10]=-1` (serial), `[-1]=0x10000` (then overwritten to 0xFFFF by the
   allocator's arg2), `[0x16]=0`, `[0x12]=-6`, `[-8]=0` (modeBits), `[0]=0`
   (mode); between the first two writes it calls
   `func_001FF308(i, 0xFFFF, 0, 0, 0, 0, 1)`.
3. Allocate the main HUD bank (`Hud_HeapAlloc(0x2800, 0, "hud.cpp", 0x115)`)
   and the aux bank (`Hud_HeapAlloc(0x1400, 0, "hud.cpp", 0x116)`) only when
   `hudBankMain` is still 0; set `hudBankEnd = main + 0x2800`,
   `hudBankBaseGp = main`, `FastMemZero16(main, 0x2800)`, and
   `main->+0x20 = 0xFF`.

0x115/0x116 are the original `__LINE__` values (277/278). Confirmed against
Lombyte's same-build recovery (`reference/Lombyte/src/assembly/textbin/
fun_001fee88.c`), which matches field-for-field.

## Symbols added

- `config/symbols.txt`: `Hud_InitBanks__Fv = 0x001FEE88`, `hudChanSlots =
  0x00199B60`, `hudBankMain = 0x0015FA00`, `hudBankEnd = 0x0015FA08`,
  `hudBankAux = 0x0015FA0C`, `hudFileStr = 0x0015F6D8` ("hud.cpp", in .lit —
  referenced, not redefined).
- `config/linker_aliases.ld`: `hudBankBaseGp = 0x0015FA04` (GP-relative
  alias; the plain words at 00/08/0C stay self-based absolute).

## Codegen findings

1. **Splat boundary split to an SN TU.** `hud.o` is one of the five GNU
   compatibility TUs, but this function needs ps2eeas: three of the four
   bank words (0x15FA00/08/0C) are self-based absolute while 0x15FA04 is a
   GPREL16 store (`sw $2, -0x71FC($28)`) in the `FastMemZero16` delay slot.
   GNU-as cannot produce that mix (it expands every bare pseudo GPREL), so
   the function is isolated into `game/hud_pre` (`[0xffe08, …]` /
   `[0xfffbc, …]`) which uses the SN default; `game/hud` keeps the GNU flag.
2. **GPREL store.** `hudBankBaseGp` is declared `extern void* hudBankBaseGp;`
   with `asm(".extern hudBankBaseGp, 4");` so ps2eeas expands the bare
   pseudo as a GPREL16 store; the linker alias pins it to 0x15FA04.
3. **Register pin defeats constant-pointer folding (the only hard part).**
   The anchor is a constant offset from the table symbol, and EGC 2.95.2
   folds it into the symbol in every plain form tried (`(int*)hudChanSlots +
   9`, an intermediate `slot`, `&slot->mode`, reordering, removing the `main`
   local — all 8-prologue-diff). Pinning the base to v1
   (`register int* tableBase asm("$3"); tableBase = (int*)hudChanSlots;
   int* anchor = tableBase + 9;`) forces the original's separate
   `lui v1, table; addiu v1, lo; addiu s0, v1, 0x24` setup and lets the
   `&hudHeap` base take a0, matching the prologue byte-for-byte.
4. **Tail:** using the bank globals directly (no `main` local) reproduces the
   original's v0-held pointer and the `bnez`+delay scheduling; a `void*`
   local instead keeps the pointer in v1 and reorders the tail (12 diffs).

## Verification (mechanical)

- `decomp_probe.py` candidate vs
  `code/_generated/nonmatchings/game/hud/func_001FEE88.s`: 308/308 bytes,
  0 diffs.
- Full `make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes
  (byte-for-byte). Raw 0x134-byte slice at file offset 0xFFE08 equal.
- Caller's regenerated asm now uses `jal Hud_InitBanks__Fv`.

## Follow-ups

- `func_001FF308` (slot allocator, 0x10C bytes) and `func_001FF418` are the
  natural next HUD targets; they will confirm the `HudChanSlot` field names
  (modeBits/mode/serial are the only established ones so far).
- `hudHeap.field_04` (the +0x04 word) is still unnamed; a future function
  that reads it (not just clears it) should establish the real name.
