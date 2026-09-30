# func_001F5448 (0x1F5448, 4 bytes) — RESOLVED: dead-tail ghost fragment

Not a function. Unreachable 4-byte fragment immediately after the blocked
parent `DrawRectOverlay_FiiiiUl` (0x1F52A0, 0x1A8, blocked 2026-09-30 as an
EGC RA wall — see draw_post_DrawRectOverlay_FiiiiUl.md).

```
0x1F5448: sw v0, 0(gp)    (0xAF820000)
0x1F544C: nop             (0x00000000, alignment padding)
```

The parent's live epilogue ends at 0x1F5444: `jr ra` at 0x1F5440 with the
GPREL store `sw v0,-0x5D00(gp)` (vu1ChainHead, 0x160F00) in the delay slot.
The fragment is the compiler's second copy of that same final head update:
EGC's two-stores-before-return scheduling kept the vu1ChainHead store alive
in the jr delay slot and placed a duplicate store of the same register v0
(the new chain head) after the jr — unreachable. The duplicate's target is
`D_00166C00`, the gp-window base word (`_gp = 0x166C00`; crt0 loads it into
$gp at boot), so the dead bytes are `sw v0,0(gp); nop`.

## Deadness (verified 2026-09-30)

- Ghidra: no function at or containing 0x1F5448; zero xrefs to it.
- `tools/deadness_scan.py 0x1F5448`: 0 reference(s) (checks jal/j/
  relative-branch targets in core.text/.text plus 32/64-bit data words in
  all other sections of the boot ELF).
- The fragment is gap bytes after the parent's ELF symbol (size 0x1A8 ends
  exactly at 0x1F5448) and before `DrawTexturedQuad` at 0x1F5450.

## Resolution (dead-tail policy, AGENTS.md)

Not a decompilation target and deliberately NOT in blocked.json. The
`INCLUDE_ASM` placeholder was replaced with a top-level raw-asm block at its
source position (code/game/draw_post_post.cpp, between
`DrawRectOverlay_FiiiiUl` and `DrawTexturedQuad`) emitting the exact two
words, following the established 989snd dead-tail pattern (top-level asm
with `.align 3` + `nonmatching`/`glabel`, family table in
notes/989snd_func_0012E078.md; that family covers the blocked-parent case
with the same top-level form).

Verified: draw_post_post.o layout identical (fragment at parent+0x1A8, next
function at +0x1B0); full build + `cmp build/boot_elf.elf
assets/boot_elf.elf` byte-identical; decomp_status 611 -> 610 targets
(active 572 -> 571).

If `DrawRectOverlay_FiiiiUl` is ever matched in C, move this tail into that
function as trailing inline assembly (the ghost is consumed by the parent,
per the dead-tail policy) and drop the top-level block.
