# func_001FF568 — dead addiu-sp tail after Hud_SetupChannelIcon

Not a function. 4-byte unreachable fragment at 0x1FF568: one
`addiu $sp,$sp,0x40` (word 0x27BD0040) plus a trailing nop, sitting right
after the matched parent `Hud_SetupChannelIcon` (func_001FF500, 0x1FF500-
0x1FF567, epilogue `jr ra` / `addiu sp,sp,0x20` delay slot). The 0x40 unit
size does NOT match the parent's 0x20 frame — same as the 989snd dead
addiu-sp tail family (see 989snd_func_0012E078.md): the original compiler
emitted these bytes after the parent's RTL and the linker placed them.

## Deadness

- `tools/deadness_scan.py 0x1FF568`: 0 references (no jal/j/relative-branch
  target, no 32/64-bit data word in any other section points here).
- No Ghidra function at 0x1FF568.

## Handling

Per the dead-tail rule, this is byte-preservation assembly, not a
decompilation target and not a blocker. The INCLUDE_ASM placeholder in
code/game/hud_post_post.cpp was replaced, at the same source position (after
the `Hud_SetupChannelIcon` C body, before the func_001FF570 INCLUDE_ASM),
with a file-scope inline asm block emitting the exact words, keeping the
`nonmatching func_001FF568, 0x4` / `glabel` / `endlabel` structure so the
linker-script-pinned symbols func_001FF568 and func_001FF568.NON_MATCHING
(both at 0x1FF568 in build/SCUS_971.99.ld) stay defined. Precedent:
code/game/actuator.cpp dead tails at 0x1E9120 and 0x1E9148.

## Verification

- Object .text offset 0x68-0x6F = 0x27BD0040, 0x00000000; parent ends at
  0x67, next function (func_001FF570) starts at 0x70 — identical to the
  original layout.
- decomp-verifier: clean `make clean && make split && make -j2` +
  `cmp build/boot_elf.elf assets/boot_elf.elf` byte-identical.
- Nonmatching count 550 -> 549.

## Neighbors in this TU (not touched)

- func_001FF570 (0x78 B): real function — scans the 13 HudChanSlot entries
  of hudChanSlots, finds the first slot whose iconId equals arg0 and that
  flag is zero, sets it; still INCLUDE_ASM (active target).
- func_001FF5E8 (0xF0 B): real function (0x20 frame, strlen + a
  FastMemCopy-style 20-byte copy from D_001E7AA8 when len < 0x50, then
  func_001165B8); still INCLUDE_ASM (active target).
- func_001FF6D8 (0x90 B region): multi-unit dead addiu-sp tail — 18
  `addiu sp,sp,N; nop` units (0x10, 0x20, 0xB0, 0x120, 0x70, 0xD0, 0x20,
  0x20, 0x50, 0xC0, 0x30, 0x40, 0x20, 0xB0, 0xA0, 0xD0, 0x70, 0x30) grouped
  by spimdis into one "function"; same ghost family, handle with inline asm
  after the parent when that region is reached.
