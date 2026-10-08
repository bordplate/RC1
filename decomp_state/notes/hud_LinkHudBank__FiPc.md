# hud LinkHudBank__FiPc (0x1FEFC0, 0x160)

Matched 2026-10-08. Source: `code/game/hud.cpp`. Real mdebug name
(`LinkHudBank__FiPc = 0x01fefc0` in config/symbols.txt).

## Semantics

Re-bases the HUD palette/texture ram entries of `bank` onto a freshly
allocated `ram` base and marks the bank as loaded.

- `bankLoads` = `hudHeap.header + 0x74` (word offset 0x1D), the per-bank
  `bankLoad` table; `pLoad = bankLoads + bank`.
- If `*pLoad == 0` (bank not yet linked): align `ram` down to 0x10
  (`(v + 0xF) & ~0xF`), store the aligned base into `*pLoad`, then walk
  `pals[prevPal..palCount[bank])` and `texs[prevTex..texCount[bank])`,
  clearing bit 31 (the flag) of each `ram` entry and adding the aligned
  base. `prev` = the previous bank's count, 0 for bank 0.
- A ram entry is an offset within the bank's ram with bit 31 as a flag;
  the re-base clears the flag and adds the new base.

Callers (jal targets found in the asset): `func_001FF120` (hud) at
0x1FF17C and `LoadHudBanks__Fv` (loaders) at 0x202C28.

## Codegen findings

1. **Bank index arithmetic**: the original computes `(header + 0x74) +
   bank * 4` — base pointer first, then the scaled index added. Indexing
   `hudHeap.header->bankLoad[bank]` directly makes EGC fold it to
   `header + (bank * 4 + 0x74)`, a different offset form. Match with
   `u32* bankLoads = (u32*)hudHeap.header + 0x1D; pLoad = bankLoads + bank;`
   (word offset 0x1D = 0x74/4).
2. **Seed placement before the guard**: `char* value = ram;` must sit in
   its own basic block BEFORE the `*pLoad == 0` test. EGC 2.95.2 keeps it
   as the `move` packed into the bnez delay slot (0x1FEFE8). If the seed
   assignment is in the same block as the alignment expression, EGC folds
   it into the aligned computation and the delay-slot move disappears.
3. cfront cannot parse `offsetof`; the constants are `#define`s
   (`HUD_RAM_ALIGN`, `HUD_RAM_ENTRY_MASK`, `HUD_BANKLOAD_WORD_OFFSET`).

## Method

Standalone EGC probes cand1–cand18 in
`working/hud_LinkHudBank__FiPc/` (now cleared); cand17/cand18 = 0 word
diffs via `tools/decomp_probe.py`. Full build + `cmp` against
`assets/boot_elf.elf` (sha1 61a44859, the re-extracted oracle whose
uniform `vram = file + 0x100000` layout the config was re-based to)
passes byte-for-byte.
