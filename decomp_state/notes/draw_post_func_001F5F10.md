# func_001F5F10 (0x1F5F10, 4 bytes) — RESOLVED: dead-tail ghost fragment

Not a function. Unreachable 4-byte fragment immediately after the blocked
parent `func_001F5AB0` (DrawOcclEffectSprite, 0x1F5AB0, 0x460 = 280 words,
blocked 2026-10-01 as an EGC RA/scheduler wall — see
draw_post_func_001F5AB0.md).

```
0x1F5F10: sw v0, 0(gp)    (0xAF820000)
0x1F5F14: nop             (0x00000000, alignment padding)
```

The parent's live epilogue ends at 0x1F5F0C: the final vu1ChainHead RMW store
`sw v0, %lo(vu1ChainHead)(at)` (absolute, base `lui at, %hi`) at 0x1F5F04, then
`jr ra` at 0x1F5F08 with `addiu sp, sp, 0x170` in the delay slot (the parent's
280th word). The fragment is the compiler's second copy of that same final head
update: EGC's two-stores-before-return scheduling kept the live head store
before the jr and placed a duplicate store of the same register v0 (the new
chain head) after the jr — unreachable. The duplicate's target is `D_00166C00`,
the gp-window base word (`_gp = 0x166C00`; crt0 loads it into $gp at boot), so
the dead bytes are `sw v0,0(gp); nop`. Same fragment as the sibling
func_001F5448 (see notes/draw_post_func_001F5448.md).

## Deadness (verified 2026-10-01)

- Ghidra: no function at or containing 0x1F5F10; zero xrefs to it.
- `tools/deadness_scan.py 0x1F5F10`: 0 reference(s) (checks jal/j/
  relative-branch targets in core.text/.text plus 32/64-bit data words in all
  other sections of the boot ELF).
- The fragment is gap bytes after the parent's ELF symbol (size 0x460 ends
  exactly at 0x1F5F10) and before `DrawUIFrame` at 0x1F5F18.

## Resolution (dead-tail policy, AGENTS.md)

Not a decompilation target and deliberately NOT in blocked.json. The
`INCLUDE_ASM` placeholder was replaced with a top-level raw-asm block at its
source position (code/game/draw_post_post.cpp, between `func_001F5AB0` and
`DrawUIFrame`) emitting the exact two words, following the established dead-tail
pattern (top-level asm with `.align 3` + `nonmatching`/`glabel`; sibling
notes/draw_post_func_001F5448.md; 989snd family table in
notes/989snd_func_0012E078.md covers the blocked-parent case with the same
top-level form). The stale queue entry was removed to match the sibling.

Verified: draw_post_post.o layout identical (fragment at 0x3cb0 = parent+0x460,
next function DrawUIFrame at +0x468); full build + `cmp build/boot_elf.elf
assets/boot_elf.elf` byte-identical; decomp_status 608 -> 607 targets.

If `func_001F5AB0` is ever matched in C, move this tail into that function as
trailing inline assembly (the ghost is consumed by the parent, per the dead-tail
policy) and drop the top-level block.
