# UpdateTieTextures (code/game/tiefunc.cpp)

- Original: 84 bytes (`0x54`) at vram `0x235840` (file offset `0x1367C0`),
  Splat placeholder `func_00235840`. Renamed `UpdateTieTextures` (C++,
  mangled `UpdateTieTextures__Fv`); the old name is kept resolvable for the
  still-assembly caller via `func_00235840 = 0x00235840;` in
  `config/linker_aliases.ld`.
- Semantics: the "active tie" texture update. Called by the main draw path
  (`DrawDebugProfiler`, 0x1F39D0) immediately after `refreshTieLights`
  (0x235898) when the tie-dye moby is present and in its manipulated state
  (`iGpffff8180 != 0`). Body:
  ```cpp
  void UpdateTieTextures(void) {
      PatchTieGifs();
      FastMemCopy(D_001E3000, D_001E4200, 0x200);
      FastMemCopy(D_001E2A00, D_001E3E00, 0x400);
      PatchTieGifs();
  }
  ```
  `PatchTieGifs` (0x235780, C linkage, still INCLUDE_ASM) walks a
  0xFFFF-terminated index table at D_001E3004 (count at D_001E3000) over the
  tie GIF structures in D_001E1700, OR'ing a 10-bit palette entry (from the
  0x200-entry u16 palette at D_001E2A00) into each GIF's color words. This
  function patches the GIFs, then writes the staged copies D_001E4200->
  D_001E3000 (0x200) and D_001E3E00->D_001E2A00 (0x400) over the structure
  and palette, then patches again with the new data.
- Match: matched on the first standalone probe
  (`decomp_state/probes/tiefunc_00235840.cpp`) with default flags — no
  scheduler or address-splitting override needed. Each `FastMemCopy` arg pair
  is a plain out-of-gp-window `lui %hi / addiu %lo` symbol load (low16 < 0x8000
  => signed split), with the size constant (`li a2,0x200/0x400`) hoisted into
  the `jal` delay slot; the two zero-arg `PatchTieGifs` calls are plain
  `jal/nop`.
- Buffers: D_001E3000 (patch count + index table), D_001E2A00 (palette), and
  the staged sources D_001E4200 / D_001E3E00 are still referenced by their
  linker-symbol names (shared with the still-assembly `PatchTieGifs`); a
  semantic rename is future work for the whole tie-texture set.
- Verification: object `UpdateTieTextures__Fv` disassembly matches the
  original instruction-for-instruction; clean `make clean && make split &&
  make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf` byte-for-byte;
  nonmatching count 712 -> 711.
