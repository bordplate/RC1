# func_001FF958 — dead addiu-sp tail after Hud_DrawChannels

Not a function. An 8-byte unreachable fragment at 0x1FF958-0x1FF95F: one
`addiu sp,sp,0x10` (word 0x27bd0010) plus an alignment nop (0x00000000),
sitting right after the matched `Hud_DrawChannels` (func_001FF780,
0x1FF780-0x1FF957, the preceding TU hud_post_post2_drawchannels.o) and right
before `GetIconFrame__Fii` (0x1FF960, the first real function of this TU).
The stray 0x10 deallocation is not a coherent live frame of any function —
same as the 989snd dead addiu-sp tail family (see 989snd_func_0012E078.md):
the original compiler emitted these bytes after the preceding RTL and the
linker placed them. The Splat boundary at 0x1FF958 lands the fragment in this
TU's .text (hud_post_post2_post.o(.text) is pinned at 0x1FF958).

## Deadness

- `tools/deadness_scan.py 0x1FF958`: 0 references (no jal/j/relative-branch
  target, no 32/64-bit data word in any other section points here).
- No Ghidra function at 0x1FF958.

## Handling

Per the dead-tail rule, this is byte-preservation assembly, not a
decompilation target and not a blocker. The `INCLUDE_ASM` placeholder in
code/game/hud_post_post2_post.cpp (first item in the file) was replaced with a
file-scope inline asm block emitting the exact 2 words, keeping the
`nonmatching func_001FF958, 0x4` / `glabel` / `endlabel` structure so the
linker-script-pinned symbols func_001FF958 and func_001FF958.NON_MATCHING
(both at 0x1FF958) stay defined. The trailing nop (0x1FF95C) pads to the
8-aligned GetIconFrame__Fii entry at 0x1FF960. The TU is a GNU-assembler TU
(Makefile: -Wa,-EL); the glabel/nonmatching/endlabel macros come from
code/include/labels.inc (pulled in via common.h -> include_asm.h), the same
mechanism the INCLUDE_ASM path uses. Precedent: code/game/hud_post_post2.cpp
func_001FF6D8 dead tail and the 989snd/ee/989snd_post.c dead tails.

## Verification

- Object head (objdump): `addiu sp,sp,16` @0, `nop` @4, `GetIconFrame__Fii`
  @8; nm shows func_001FF958 / func_001FF958.NON_MATCHING @0 and
  GetIconFrame__Fii @8.
- Clean: `make clean && make split && make -j2 && cmp build/boot_elf.elf
  assets/boot_elf.elf` byte-identical.
- Nonmatching count 544 -> 543.
