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

## Message-table decode (2026-10-05, refactor freeze_mode_freezeInit_msgids)
The `msg_string` ids are indexes into the runtime-loaded help table (NOT present in the boot
ELF: `HelpMsgs`@0x15F6A0 is a pointer, `helpMsgData`/`helpOffsetTable`@0x15EF60/64 are
0xCDCDCDCD-filled at boot). The boot startlevel function (0x1EA830) does
`helpOffsetTable = helpMsgData; for (set=0..7) Help_LoadMsgs(set); ...; Help_LoadMsgs(0);`, so the
text file layout is `[8 set offsets][set 0 node][set 1 node]...`.

- `assets/globals/all_text.bin`: the global set collection (630784 bytes). First 8 ints are the
  set offsets `[0x20, 0x1D040, 0x1D050, 0x3CAE0, 0x5B1B0, 0x7A5D0, 0x99830, 0x99850]`; set 0's
  node is at file offset 0x20 = `[count][w1][entries...]` with entries as 8-byte
  `(textoff, id)` pairs and textoffs relative to the set node. (The `count` word does not equal
  the entry count; the table also contains `(0xFFFFFFFF, 0)` gap pairs and a trailing
  12-byte `(len, textoff, id)` block for ids 0x4E20-0x4E33.)
- `assets/levels/<lvl>/help_messages_<lang>.bin`: per-level set, same entry format (node at 0x00).
  The id space and texts are IDENTICAL across every level and language file checked (US/UK/F/DE/
  ES/IT/JP/KR for levels 0-18), so the ids are a shared global namespace.

Decoded US-English texts (id -> text):
  0x4F6E  "Quit Race?"
  0x5229  "Quit?"
  0x5248  "\x10 Quit"
  0x5249  "\x11 Continue"
  0x524A  "\x10 Continue"
  0x4EE0  "\x12 Exit"
  0x4E2B  "When this icon appears, your progress is being saved.\x01\x01While this icon is on
           screen, do not remove the Memory Card (PS2) or turn off the power."
The leading `\x10`/`\x11`/`\x12` bytes are structural (identical in every language) and are font
character codes that select button symbols: the glyph metric tables (fontSmallGlyphs@0x1DF050,
fontMediumGlyphs@0x1DF3F0, fontLargeGlyphs@0x1DF790) are indexed directly by char code with
stride 4 (width at +3), and 0x10-0x13 all sit at texture row y=96 (button-glyph row), so they
render as the circle/cross/triangle button icons rather than the text. (Which of 0x10/0x11/0x12
is circle vs cross vs triangle is taken from the in-game dialog: the pause "Quit?" dialog pairs
0x4EE0/0x524A, the "Quit Race?" dialog pairs 0x5248/0x5249; 0x10 is shared by the two "Continue"/
"Quit" circle lines.)

Named in freeze.cpp as MSG_QUIT_RACE / MSG_QUIT / MSG_BTN_QUIT / MSG_BTN_CONTINUE_CROSS /
MSG_BTN_CONTINUE_CIRCLE / MSG_BTN_EXIT / MSG_AUTOSAVE_WARNING. Sound group 0x1D is a group
BITMASK (snd_PauseAllSoundsInGroup forwards the u_int straight to the IOP); per-bit meanings are
not established, so it is named FREEZE_SOUND_GROUPS without asserting specific groups. The
default-case countdown 0x78 is stored raw (no func_001F96F8 scaling) and named
FREEZE_DEFAULT_COUNTDOWN_FRAMES. Pure `#define` substitution: freeze.o rebuilt, function 0/408
bytes and jtbl 0/32 bytes vs original, full boot ELF cmp byte-identical.
