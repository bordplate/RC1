# mode_freezeInit (0x1FBAB8) — MATCHED (2026-10-05)

Function: `code/game/freeze.cpp` : `mode_freezeInit` (VRAM 0x1FBAB8, size 0x198).
Original C++-mangled symbol is `mode_freezeInit__Fii` (the sibling `DrawDialogText__Fv` /
`UpdateModeFreeze__Fv` in the same TU are mangled `__Fv`, so this file was C++-mangled; a plain
`void mode_freezeInit(int,int)` definition emits exactly `mode_freezeInit__Fii` — do NOT use
`extern "C"`, which would emit the unmangled name the callers do not reference).

## Semantics
```
void mode_freezeInit(int dialog, int restorePage):
  if (GameMode != 3) { snd_PauseAllSoundsInGroup(0x1D); music_Pause(0); }
  Freeze.prevGameMode = GameMode;   // GameMode=0x15F604 (3=pause,4=freeze)
  Freeze.restorePage = restorePage;
  GameMode = 4;
  Freeze.mode = dialog;             // stored in the bounds-check (beqz) DELAY SLOT
  switch (dialog) {                 // DENSE 7-case (0..6); single `sltiu` is the guard
    ...
  }
```
Global `Freeze` @0x193300 (named in config/symbols.txt). Fields (all u32 — the original stores
the 32-bit msg_string pointers with `sw`, NOT 64-bit `sq`): +0x00 mode, +0x04 countdown, +0x08
query, +0x0C ans1, +0x10 ans2, +0x14 prevGameMode, +0x18 restorePage, +0x1C f1C, +0x20 f20,
+0x24 f24, +0x28 f28. (Field names from the Deadlocked reference freeze.h; query/ans1/ans2 hold
32-bit message-text addresses.)

Callees: snd_PauseAllSoundsInGroup (C-linkage, `extern "C"`, 0x12e3e8), msg_string__Fi (C++,
0x1fdd10), music_Pause__Fi (C++, 0x216050), func_001F96F8 (C-linkage), func_001FED30 (C-linkage).

## KEY CODEGEN LEARNING (the actual match obstacle)
EGC lays out dense-switch case bodies in **SOURCE order**, not case-value order. The original
`.text` order is: **case 0, case 2, case 1/4, case 5, case 3, case 6, default** (case 2 at
0x1FBB84 comes BEFORE case 1/4 at 0x1FBBA0). So the C source must put `case 2:` immediately after
`case 0:`, ahead of `case 1:/case 4:`. Getting this order wrong leaves the `.text` instruction
mnemonics looking right but permutes the case-block positions, so the linked jtbl entries (absolute
case-handler addresses) do not match the original bytes -> cmp fails at the jtbl.

Also: cases 3 and 5 have IDENTICAL tails; write each with its FULL body (case 5 does
`func_001FED30(0x4E2B)` then the shared two-`func_001F96F8(0x1E)` body; case 3 just the shared
body). EGC 2.95.2 tail-merges the identical portion — do NOT use a `goto`; a goto form compiles
to a fall-through that EGC schedules differently.

## THE TOOLING (what unblocked this — see commit "tooling: place freeze.o switch jtbls...")
The original keeps this dense-switch jumptable (jtbl_001E78D0, 0x20 bytes) INSIDE the data segment
at 0x1E78D0, which Splat models as one raw blob (data.data.o). EGC emits the jtbl into the object's
`.rodata`. The fix:
- config/RC1.yaml: carve the jtbl hole out of the data blob as its own `data_freeze` segment
  (ROM 0xE8850 / VRAM 0x1E78D0, size = up to the next segment) plus a `data_suffix` segment
  (0xE8870 / 0x1E78F0). Splat gives each its own pinned-VMA output section.
- tools/patch_freeze_rodata_ld.py (run from the Makefile link rule before every ld): if
  build/code/game/freeze.o has a non-empty `.rodata`, rewrite the generated SCUS_971.99.ld to place
  `freeze.o(.rodata)` in the `.data_freeze` output section (replacing the raw-blob input) and drop
  it from the `.text` rodata region, appending a TOP-LEVEL `ASSERT(ADDR(.data_freeze)+SIZEOF(
  .data_freeze) == 0x1E78F0, ...)` (ASSERT must be OUTSIDE the SECTIONS block and on ONE line —
  this ld rejects it inside the block / spanning lines). Otherwise (whole TU still INCLUDE_ASM) it
  restores the raw-blob input and the `.text` line. Idempotent, stdlib-only ELF32 section parse.

Two-state behavior: while only mode_freezeInit is decompiled, freeze.o(.rodata) is exactly the
0x20 jtbl and fills the hole. When DrawDialogText / UpdateModeFreeze are later decompiled, their
jtbls/D-blocks join freeze.o(.rodata) and the `data_freeze` hole must be widened (YAML start of
`data_freeze` stays 0xE8850; move `data_suffix` start/vram to the new end) and the ASSERT bound
updated to match.

## Verification
`make clean && make split && make -j2` then `cmp build/boot_elf.elf assets/boot_elf.elf` => byte
identical. Function `.text` (102 instr) and the linked jtbl both match the original.
