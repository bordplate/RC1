# func_001F0BC8 (code/game/draw.cpp) — dead tail, resolved 2026-09-25

`func_001F0BC8` at vram `0x001F0BC8` is one of the ~46 dead-tail "phantom
functions": an unreachable 4-byte fragment

```
0x1F0BC8: addiu sp,sp,0x2A0
0x1F0BCC: nop
```

sitting at an 8-aligned address right after the preceding function's epilogue
(one alignment nop at 0x1F0BC4 before it). It is NOT an independent
decompilation target.

## Parent

`draw_resetTextureDmaState__Fv` (0x1F0B88, 0x40 bytes, matched C in
draw.cpp): stores 1 into the 20-int `drawTextureDmaState` array (0x18A2B0)
backwards from index 19 to 0. The parent has no frame of its own, so the
0x2A0 (672) deallocation matches no frame — same artifact class as the
989snd dead `addiu sp,sp,N` tails (see notes/989snd_func_0012E078.md, whose
t1-t10/p1-p5 probes established that EGC 2.95.2 never regenerates a dead
frame deallocation after the epilogue).

## Deadness

`tools/deadness_scan.py 0x1F0BC8` → 0 references (no jal/j target, no
relative-branch target, no 32/64-bit data word in any section points at it).
Ghidra has no function there.

## Replacement

The orphan `INCLUDE_ASM(..., func_001F0BC8)` in draw.cpp was replaced with a
file-scope raw-asm block (same pattern as 989snd_mid.c's func_0012DF18):
`.align 3` + `nonmatching`/`glabel`/`endlabel` + `.word 0x27bd02a0` +
`.word 0`, emitting the exact fragment and its leading alignment nop at the
same section offset. The `func_001F0BC8` and `func_001F0BC8.NON_MATCHING`
symbols are still defined by the object, so the .ld pins at 0x1F0BC8 remain
satisfied.

## Verification

- draw.o `.text`[0:0x48] == original 0x1F0B88..0x1F0BD0 byte-for-byte except
  the two HI16 reloc fields of the parent's `drawTextureDmaState` lui/addiu
  (linker-resolved; the linked ELF is exact).
- Ghost region object == original: `00000000 a002bd27 00000000`.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- Nonmatching count 647 → 646.

No blocked.json entry (dead-tail ghosts are byte-preservation assembly, not
blockers).
