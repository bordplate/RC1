# func_0022E180 — dead addiu-sp tail after func_0022DF40

Not a function. 4-byte unreachable fragment at 0x22E180: one
`addiu $sp,$sp,0x10` (word 0x27BD0010) plus a trailing nop, sitting after the
five boundary nops (0x22E16C-0x22E17C) at the end of the parent
`func_0022DF40`'s region (0x22DF40-0x22E16B, 0x170 frame; still INCLUDE_ASM).
The 0x10 unit size does NOT match the parent's 0x170 frame — same as the
989snd dead addiu-sp tail family (see 989snd_func_0012E078.md): the original
compiler emitted these bytes after the parent's RTL and the linker placed
them. The next real function is the matched `space_beginLoad`
(func_0022E188) at 0x22E188.

## Deadness

- `tools/deadness_scan.py 0x22E180`: 0 references (no jal/j/relative-branch
  target, no 32/64-bit data word in any other section points here).
- No Ghidra function at 0x22E180.

## Handling

Per the dead-tail rule, this is byte-preservation assembly, not a
decompilation target and not a blocker. The INCLUDE_ASM placeholder in
code/game/space.cpp was replaced, at the same source position (after the
func_0022DF40 INCLUDE_ASM, before the `space_beginLoad` C body), with a
file-scope inline asm block emitting the exact words, keeping the
`nonmatching func_0022E180, 0x4` / `glabel` / `endlabel` structure so the
linker-script-pinned symbols func_0022E180 and func_0022E180.NON_MATCHING
(both at 0x22E180 in build/SCUS_971.99.ld) stay defined. Precedent:
code/game/actuator.cpp dead tails at 0x1E9120 and 0x1E9148, and
code/game/hud_post_post.cpp func_001FF568.

## Verification

- Object .text offset 0x370-0x377 = 0x27BD0010, 0x00000000; parent
  func_0022DF40 ends at 0x36F (region end after boundary nops), next function
  space_beginLoad__Fi starts at 0x378 — identical to the original layout
  (objdump of assets/boot_elf.elf: 22e180: 27bd0010, 22e184: 00000000).
- Full `make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf`
  byte-identical.
- Nonmatching count 548 -> 547.

## Neighbors in this TU (not touched)

- func_0022DF40 (0x240 B region): real function (0x170 frame, camera/VU
  loop); still INCLUDE_ASM (active target).
- func_0022E1A8 (0x278 B): real function; still INCLUDE_ASM (active target).
- space_beginLoad (0x22E188, 0x1C B): matched (formerly func_0022E188).
