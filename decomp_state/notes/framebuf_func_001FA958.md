# func_001FA958 (0x1FA958, 0x1C bytes) — RESOLVED: dead-tail ghost fragment

Not a function. Unreachable 28-byte fragment immediately after the blocked
parent `func_001FA860` (FastMapMaskRLE, 0x1FA860, 0xF8, blocked 2026-10-03
as handwritten trapping signed arithmetic — see
framebuf_func_001FA860.md), a multi-unit dead `addiu sp,sp,N` tail of the
family documented in notes/989snd_func_0012E078.md.

```
0x1FA958: addiu sp,sp,0xE0   (0x27BD00E0)
0x1FA95C: nop                (0x00000000)
0x1FA960: addiu sp,sp,0x70   (0x27BD0070)
0x1FA964: nop                (0x00000000)
0x1FA968: addiu sp,sp,0x80   (0x27BD0080)
0x1FA96C: nop                (0x00000000)
0x1FA970: addiu sp,sp,0x90   (0x27BD0090)
0x1FA974: nop                (0x00000000, alignment padding before 0x1FA978)
```

The parent's live epilogue ends at 0x1FA957: `jr ra` at 0x1FA950 with
`addiu v0,zero,-1` in the delay slot. The fragment is gap bytes between the
parent's ELF symbol (size 0xF8 ends exactly at 0x1FA958) and
`SetupFS_AA_buffer__Fiiiiii` at 0x1FA978. The unit sizes (0xE0/0x70/0x80/0x90,
sum 0x240) do not match the parent's frame, consistent with the documented
multi-unit dead-tail behavior where the original compiler emitted dead
`addiu sp,sp,N` sequences after the function's RTL and the linker placed
them. Local EGC never emits such sequences.

## Deadness (verified 2026-10-03)

- Ghidra: no function at or containing 0x1FA958; zero xrefs to it.
- `tools/deadness_scan.py 0x1FA958`: 0 reference(s) (checks jal/j/
  relative-branch targets in core.text/.text plus 32/64-bit data words in
  all other sections of the boot ELF).
- objdump of assets/boot_elf.elf confirms the byte sequence above.

## Resolution (dead-tail policy, AGENTS.md)

Not a decompilation target and deliberately NOT in blocked.json. The
`INCLUDE_ASM` placeholder was replaced with a top-level raw-asm block at its
source position (code/game/framebuf.cpp, between `func_001FA860` and
`SetupFS_AA_buffer__Fiiiiii`) emitting the exact eight words (the seven
fragment words under the glabel plus the alignment nop after it), following
the established dead-tail pattern (e.g. notes/draw_post_func_001F5448.md).

Verified: framebuf.o layout identical (fragment at parent+0xF8, next
function at +0x108 offset / 0x1FA978); full build + `cmp build/boot_elf.elf
assets/boot_elf.elf` byte-identical; decomp_status 584 -> 583 targets
(active 537 -> 536).

If `func_001FA860` is ever matched in C, move this tail into that function
as trailing inline assembly (the ghost is consumed by the parent, per the
dead-tail policy) and drop the top-level block.
