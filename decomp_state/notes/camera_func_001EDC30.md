# func_001EDC30 (0x1EDC30, 0x1C bytes) — dead-tail ghost fragment, inline-asm byte-preserved

Resolved 2026-09-25 by converting the `INCLUDE_ASM` placeholder in
`code/game/camera.cpp` (the last item in camera.o) to top-level inline
assembly that emits the exact dead bytes. Count 648 -> 647; clean
`make && cmp build/boot_elf.elf assets/boot_elf.elf` passes.

## What it is

NOT a function. Unreachable 7-word fragment immediately after the matched
`UpdateCamera__Fv` (0x1EDAA8, size 0x188), whose epilogue ends at 0x1EDC30:

```
0x1EDC28: jr ra
0x1EDC2C: addiu sp,sp,0x70   (delay slot)
```

Fragment bytes (no prologue, no return, nothing reaches it):

```
0x1EDC30: 0x00a0102d  move   v0,a1
0x1EDC34: 0x00000000  nop
0x1EDC38: 0x27bd0020  addiu  sp,sp,0x20
0x1EDC3C: 0x00000000  nop
0x1EDC40: 0x00000000  nop
0x1EDC44: 0x00000000  nop
0x1EDC48: 0xac400004  sw     zero,4(v0)
```
(one alignment nop at 0x1EDC4C before the next object's function at 0x1EDC50)

The `move v0,a1` / dead `addiu sp,sp,0x20` / dead `sw zero,4(v0)` is jumbled
dead RTL the original EGC emitted after the parent's return; the decompiled
matched `UpdateCamera` is void and does not regenerate it.

## Deadness (verified)

- Ghidra: no function at or containing 0x1EDC30.
- `tools/deadness_scan.py 0x1EDC30` -> 0 reference(s) (raw jal/j/branch/word scan).
- No prologue (`addiu sp,sp,-N`/`sq`) and no epilogue (`jr ra`/`lq`) -> cannot be a real function.
- 8-aligned address right after a `jr ra` epilogue -> classic dead-tail placement
  (same family as the 989snd `addiu sp,sp,N` tails, but with a move + store).

## Treatment

Per the dead-tail policy (AGENTS.md), not a decompilation target and not a
blocker: the bytes are preserved with top-level inline asm at the same source
position, reproducing the generated layout exactly (`.section .text` /
`.set noat noreorder` / `.align 3` / `nonmatching+glabel` / 7 `.word` /
`endlabel` / `.set reorder at`), the same form used for the 989snd_mid_byloc
tail. Same source position keeps the intra-TU layout and full parity intact.
