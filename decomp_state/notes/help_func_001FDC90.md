# func_001FDC90 (0x1FDC90, 4 bytes) — RESOLVED: dead-tail ghost fragment

Not a function. Unreachable 4-byte fragment immediately after the matched
parent `Help_AdvanceState` (func_001FDC08, 0x1FDC08, 0x88, matched
2026-10-07, commit a773c1c).

```
0x1FDC90: addiu sp,sp,0x10    (0x27BD0010)
0x1FDC94: nop                 (alignment padding)
0x1FDC98: nop                 (alignment padding)
0x1FDC9C: nop                 (alignment padding before Help_FindIndex @ 0x1FDCA0)
```

The parent's matched body takes no stack frame at all (prologue starts
`lui v1,%hi(g_helpState)`; epilogues are bare `jr ra; nop`), so the
`addiu sp,sp,0x10` cannot be live epilogue code: it is a dead stack
deallocation the original compiler emitted after the function's RTL
(same family as the 989snd multi-unit `addiu sp,sp,N` tails — see
notes/989snd_func_0012E078.md — where the unit size does not follow any
visible function). The three trailing nops are inter-function alignment
padding: the next symbol `Help_FindIndex` starts at 0x1FDCA0, 8-aligned.

## Deadness (verified 2026-10-07)

- Ghidra: no function at or containing 0x1FDC90.
- `tools/deadness_scan.py 0x1FDC90`: 0 reference(s) (checks jal/j/
  relative-branch targets in core.text/.text plus 32/64-bit data words in
  all other sections of the boot ELF).
- Raw original ELF word at 0x1FDC90: 0x27BD0010 (`addiu sp,sp,16`),
  0x1FDC94-0x1FDC9F: three zero words (objdump of assets/boot_elf.elf).

## Resolution (dead-tail policy, AGENTS.md)

Not a decompilation target and deliberately NOT in blocked.json. The
orphan `INCLUDE_ASM` placeholder was replaced with a top-level raw-asm
block at its source position (code/game/help.cpp, between
`Help_AdvanceState` and `Help_FindIndex`) emitting the exact four words,
keeping the `func_001FDC90` / `func_001FDC90.NON_MATCHING` symbols that
the linker map still references, following the established pattern of
notes/draw_post_func_001F5448.md and notes/draw_post_func_001F5F10.md.

Verified: help.o .text word-diff against the original shows the ghost
region (0x1FDC90-0x1FDC9F) byte-identical (all other object words differ
only in pre-link relocation hi-fields); full build + `cmp
build/boot_elf.elf assets/boot_elf.elf` byte-identical. Nonmatching count
569 -> 568.
