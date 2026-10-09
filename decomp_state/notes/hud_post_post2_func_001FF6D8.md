# func_001FF6D8 — dead addiu-sp tail after setMessageText

Not a function. A 0x8C-byte unreachable fragment at 0x1FF6D8-0x1FF760:
eighteen `addiu sp,sp,N` units (0x10,0x20,0xB0,0x120,0x70,0xD0,0x20,0x20,
0x50,0xC0,0x30,0x40,0x20,0xB0,0xA0,0xD0,0x70,0x30) with interleaved nops,
sitting right after the matched `setMessageText` (func_001FF658, 0x1FF658-
0x1FF6D7). None of the unit sizes is a coherent live frame — same as the 989snd
dead addiu-sp tail family (see 989snd_func_0012E078.md): the original compiler
emitted these bytes after the preceding RTL and the linker placed them.

## Deadness

- `tools/deadness_scan.py 0x1FF6D8`: 0 references (no jal/j/relative-branch
  target, no 32/64-bit data word in any other section points here).
- No Ghidra function at 0x1FF6D8.

## Handling

Per the dead-tail rule, this is byte-preservation assembly, not a
decompilation target and not a blocker. The `INCLUDE_ASM` placeholder in
code/game/hud_post_post2.cpp (at the same source position, after the
setMessageText C body, before the func_001FF780 INCLUDE_ASM) was replaced with
a file-scope inline asm block emitting the exact 35 words, keeping the
`nonmatching func_001FF6D8, 0x8C` / `glabel` / `endlabel` structure so the
linker-script-pinned symbols func_001FF6D8 and func_001FF6D8.NON_MATCHING
(both at 0x1FF6D8) stay defined. A trailing nop (0x1FF764) pads to the
8-aligned `hud_updateMessageTimer` entry at 0x1FF768. Precedent:
code/game/hud_post_post.cpp func_001FF568 dead tail and the func_001FF5E8
dead tail handled in hud_post_post2_setMessageText.md.

## Verification

- Full: `make -j2 && cmp build/boot_elf.elf assets/boot_elf.elf`
  byte-identical.
- Nonmatching count 546 -> 545.
