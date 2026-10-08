# func_001FF120 (0x1FF120, 8 bytes) — RESOLVED: dead-tail ghost fragment

Not a function. Unreachable 8-byte fragment immediately after the matched
parent `LinkHudBank__FiPc` (0x1FEFC0, 0x160, matched 2026-10-08, commit
9d878ff), whose epilogue is `jr $ra; nop` at 0x1FF118-0x1FF11C.

```
0x1FF120: sw  $zero, 0x74($v0)   (word 0xAC400074)
0x1FF124: nop                    (alignment padding before Hud_SendResidentBank @ 0x1FF128)
```

`0x74` is the byte offset of the bankLoad field block within HudHeader
(HUD_BANKLOAD_WORD_OFFSET 0x1D words, see code/game/hud.cpp), so the
fragment is a dead zero store to a bankLoad word of whatever header pointer
sits in v0 — a leftover of an original-source store the original compiler
emitted after the function's RTL (same dead-tail family as the `sw v0,
-0x3FE0(gp)` vuchain ghosts and the store-of-an-arg-field variants listed in
AGENTS.md). The trailing nop is inter-function alignment padding: the next
symbol `Hud_SendResidentBank__FiPcb` starts at 0x1FF128, 8-aligned.

## Deadness (verified 2026-10-08)

- Ghidra: no function at or containing 0x1FF120; no xrefs to it.
- `tools/deadness_scan.py 0x1FF120`: 0 reference(s) (checks jal/j/
  relative-branch targets in core.text/.text plus 32/64-bit data words in
  all other sections of the boot ELF).
- Raw original ELF words (file offset 0xFF120, vram = file + 0x100000):
  0xAC400074, 0x00000000 (objdump of assets/boot_elf.elf).

## Resolution (dead-tail policy, AGENTS.md)

Not a decompilation target and deliberately NOT in blocked.json. The
orphan `INCLUDE_ASM` placeholder was replaced with a top-level raw-asm
block at its source position (code/game/hud.cpp, between `LinkHudBank`
and `Hud_SendResidentBank`) emitting the exact two words, keeping the
`func_001FF120` / `func_001FF120.NON_MATCHING` symbols.

Verified: hud.o .text word-diff against the original shows the ghost
region (0x1FF118-0x1FF127) byte-identical (the only other diff in the
window is the pre-link R_MIPS_HI16 hi-field of `lui $2,%hi(hudHeap)` in
Hud_SendResidentBank, filled at link time); full build + `cmp
build/boot_elf.elf assets/boot_elf.elf` byte-identical. `make split`
reclassified the fragment to code/_generated/matchings/game/hud/
(stale nonmatchings copy removed). Nonmatching count 560 -> 559.
