# Hud_SetChannelModeBySerial (func_001FF570)

- Address: 0x1FF570, size 0x74 (29 words)
- Matched: 2026-10-09
- Source: `code/game/hud_post_mode.cpp` (isolated Splat segment, SN assembler)
- Header: `code/include/hud.h` (`Hud_SetChannelModeBySerial ... asm("func_001FF570")`)

## Semantics

Finds the `hudChanSlots` entry whose `serial` field (offset 0x64) matches the
`serial` argument, then sets its `pendMode` (offset 0x24) to `mode`. If the
slot is not currently `pending` (offset 0x68 is 0), its active `mode` (offset
0x04) is set to `mode` as well. Passing `mode == 0` clears the channel's mode.
`HUD_SLOT_COUNT` is 13.

Body:
```cpp
int i = 0;
while (i < HUD_SLOT_COUNT && hudChanSlots[i].serial != serial) {
    i++;
}
if (i < HUD_SLOT_COUNT) {
    hudChanSlots[i].pendMode = mode;
    if (hudChanSlots[i].pending == 0) {
        hudChanSlots[i].mode = mode;
    }
}
```

EGC peels only the FIRST iteration of the serial-scan loop, producing the
pre-check `lui/move/addiu/lw; beq` prologue followed by a `.p2align 3` loop
head that reloads the base and re-tests `i < 13`. Writing the loop as a plain
`while` from `i = 0` reproduces it; a manual two-iteration unroll does not.

## Why a Splat boundary split was required (the `#nop` / assembler finding)

This function is the reason it was split out of `hud_post_post.cpp` into its
own TU. EGC's compiler output (`.s`) contains a **`#nop` scheduler hole** in
the loop body, between the `lw $2,0($3)` (serial load) and the `bnel` back-edge:

```
lw    $2,0($3)
#nop                      <- scheduler hole
.set noreorder
.set nomacro
bnel  $2,$4,$L11
addu  $6,$6,1             <- delay slot (i++)
.set macro
.set reorder
```

The assembler decides whether that hole becomes a real instruction:

- **ps2eeas (SN, `-snas`)** fills the hole with a `nop` (word 0x00000000).
  The function is 0x74 bytes and matches the original byte-for-byte.
- **GNU as (`-Wa,-EL`)** treats `#nop` as a comment and emits **no** nop. The
  function is 0x70 bytes and every later branch target shifts by 4 (19 word
  diffs: `beq +0x24` vs `+0x28`, `beqz +0x40` vs `+0x44`, `bnel -5` vs `-6`,
  plus the trailing `move v0,zero` landing where the pad nop belongs).

The compiler `.s` is byte-identical in both cases (verified by diff); only the
assembler differs. So the SN assembler is mandatory for this function.

But `hud_post_post.o` is a **GNU-compatibility TU** (see the Makefile override
list and AGENTS.md): its other source forms rely on GNU macro expansion /
scheduling, so the whole TU cannot be flipped to SN. Per the documented
"conflicting verified flags" policy, the fix is a Splat boundary split that
gives this one function its own SN TU.

## The split

`config/RC1.yaml` `.text` subsegments (file offsets; vram = file + 0x100000):

```
- [0xff500, cpp, game/hud_post_post]    # [0x1FF500, 0x1FF570) Hud_SetupChannelIcon + dead tail (GNU)
- [0xff570, cpp, game/hud_post_mode]    # [0x1FF570, 0x1FF5E8) this function (SN, default)
- [0xff5e8, cpp, game/hud_post_post2]   # [0x1FF5E8, 0x201520) the rest (GNU)
```

- `hud_post_post.cpp` was truncated to `Hud_SetupChannelIcon` + the
  func_001FF568 dead-tail inline asm (its own 0x70 region, unchanged).
- `hud_post_mode.cpp` (new) holds this function + one trailing `asm("nop")`
  pad (the C body emits 0x74, ending at 0x1FF5E4; the next function sits at
  0x1FF5E8, so 4 bytes of pad are needed).
- `hud_post_post2.cpp` (new) holds the remainder (func_001FF5E8 ...
  draw_bootImage__Fi, incl. the matched `hud_updateMessageTimer` =
  func_001FF768); its INCLUDE_ASM paths were re-pointed to
  `.../game/hud_post_post2`.

Makefile: `hud_post_post2.o` was added to the GNU-compatibility override list;
`hud_post_mode.o` is left on the default (SN) assembler. Objects are
auto-discovered via `SRC_CPP`, so no object-list edit was needed.

## Verification

- `tools/decomp_probe.py ... --assembler snas` on the candidate: size 0x74 vs
  0x74, **0 word diffs**.
- `tools/decomp_probe.py ... --assembler gnu`: size 0x70 vs 0x74, 19 word diffs
  (confirms the assembler dependency).
- After the split: clean `make split && make`, and
  `cmp build/boot_elf.elf assets/boot_elf.elf` is byte-identical (parity
  green).
