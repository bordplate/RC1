# InitMemSlots__Fv (0x2015D8) — matched 2026-10-10

Lays out the level memory map `levelMem` (0x1940C0) in `code/game/initonce.cpp`.
Void function, 0x78 bytes (30 words), 3 callers (0x1EA878, 0x2018C0, 0x230FE4).

## Semantics

```
base     = (text_VRAM_END + 0x3FFF) & 0xFFFFC000   // 16K-aligned end of boot text
f04      = base
f08      = base + currentVuChain                   // currentVuChain = 0x160F0C (VU chain size)
f0c      = f08 + currentVuChain                    // = hudHeapBase (0x1940CC); HUD heap spans 0x64000 (hud.cpp)
f10      = f0c + 0x64000
f14      = f10 + 0x30000                           // level descriptor base (loader reads fields, decompresses at +0x400000)
f18      = f14                                     // loader later advances f18 = f14 + placed size
f20..28  = 0x7000000 / 0x7100000 / 0x7200000       // reserved occlusion regions
```

`f20` (0x1940E0) is aliased by the `occlSamplePoints` symbol; the occlusion
sampler reads it as a pointer to sample points. `f04`/`f0c` are also aliased
by the `MemSlots` (0x1940C4) / `hudHeapBase` (0x1940CC) symbols.

## Naming evidence (Deadlocked, reference/dl)

DL's `game_dl/initonce.cpp` has the same `InitMemSlots` with a `MemSlots`
struct: `base` + `vu1_chain[2]` fields, then a chain of region fields
(`tie_cache = vu1_chain[1] + vu1_bufSize; moby_joint_cache = + 0x100000;
joint_cache_entry_list; level_base = ... + 0x2000; level_end = level_base`),
and the exact triple `occl_points = 0x7000000; occl_grids = 0x7100000;
debug = 0x7200000`. The RC1 loader's use of f14 (descriptor field reads +
`FastDecompress(f14 + 0x400000, ...)`) matches DL's `level_base` use exactly.
Named accordingly: `base`, `level_base`, `level_end`, `occl_points`,
`occl_grids`, `debug`. The middle fields (f04–f10) keep offset-based names:
their roles (tie cache / joint cache / post-HUD-heap region) are only
partially supported, and f0c's strong RC1 evidence is the HUD heap.

## Match-sensitive form

The body is a straight line of 10 stores; the matching shape (probe-verified,
0 diffs) writes each field directly through the struct with NO local
variables — locals (cand1/cand5/cand7), register pins (cand3), operand
reversal (cand9), or statement reordering (cand2/cand6) all produce 19–26
word diffs from EGC's register allocation / store-ordering. Key forms:

- `base = ((u32)(text_VRAM_END + LEVEL_ARCHIVE_ALIGNMENT - 1) &
  ~(LEVEL_ARCHIVE_ALIGNMENT - 1));` — the named-constant form folds to the
  same `lui; addiu 0x3FFF; andi 0xC000` as the literals (same idiom as
  matched `startlevel__Fv` in bmain.cpp).
- `levelMem.field_0x18 = levelMem.level_base;` — EGC CSEs the f14 read (the
  original reloads it, not `move`).
- Occlusion stores must go through `levelMem` fields (offsets 0x20/0x24/0x28
  from the one materialized base register); a separate symbol for 0x1940E0
  would emit its own `0x40E0`-offset bases and break the match.
- initonce.o is SN-assembled, so plain `extern int currentVuChain` expands
  to the self-based `lui/lw` the original uses.
- Natural C++ name `InitMemSlots` mangles to the target `InitMemSlots__Fv`;
  no asm override needed.

## Verification

Probe: 0x78/0x78, match, 0 diffs (cand_final.c + real levelmem.h,
`--define text_VRAM_END=0x23d360`). Full clean `make split && make -j2` +
`cmp build/boot_elf.elf assets/boot_elf.elf` passes (shared header
levelmem.h touched; bmain.o/help.o rebuilt).

## Follow-ups

- `Hud_HeapReset` (hud.cpp) uses the raw HUD-heap size `0x64000`; recorded in
  refactor.json (should reuse the named size from this layout).
- f04–f10 roles (tie cache / joint cache chain) remain offset-named until the
  consumers are decompiled.
