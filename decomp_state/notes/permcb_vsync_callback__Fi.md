# vsync_callback__Fi (code/game/permcb.cpp)

- Original: 64 bytes (`0x40`) at vram `0x12F1C8` (file offset `0x2F148`),
  symbol `vsync_callback__Fi` (C++ `(int)`). The name comes from the original
  ELF symbol table; no rename was needed. The argument is unused.
- Semantics: vertical-sync callback. Body:
  ```cpp
  int vsync_callback(int) {
      frm_vsync_cnt += 1;
      D_0015ED50 = frm_clock_time + *(volatile unsigned int*)0x10000800;
      return 0;
  }
  ```
  Increments the vsync counter `frm_vsync_cnt` (0x15ED48) and stores
  `frm_clock_time` (0x15ED40) plus a tick value read from 0x10000800 into
  `D_0015ED50`. Returns 0 (the original hoists `move v0,zero`).
- Registration: no pointer literal for 0x12F1C8 exists in the boot ELF
  (verified by raw-encoding scan of `j`/`jal` targets and lui/addiu pairs in
  core.text, .text and the overlays), so it is registered outside the boot
  image (overlay or library). Not needed for parity.
- 0x10000800: in the EE VU0 microprogram region (0x10000000), which has no
  ELF symbol. Other boot-image users: `InitOnce__Fv` writes
  `*(u32*)0x10000810 = 130; *(u32*)0x10000800 = 0;` (0x201960-0x201978), and
  `DrawDebugProfiler` reads `*(u32*)0x10000800` at 0x1F4064 and passes it to
  `func_001FA6C0`. Both show the same unfused `lui/ori + offset-0 access`
  shape, consistent with a value the VU side publishes. The semantic meaning
  of the value is unconfirmed.
- Globals: `frm_vsync_cnt` / `frm_clock_time` are original ELF names
  (config/symbols.txt). `D_0015ED50` has no original name; in the boot image
  only this function writes it and nothing reads it (consumer likely in a
  level overlay). All three start at 0 (.core_lit). `frm_clock_time` is
  never written in the boot image, so its writer is unconfirmed.
- Codegen findings (the match-sensitive part):
  - `frm_vsync_cnt` / `frm_clock_time` (in-gp-window): a plain `extern`
    compiles to a single GPREL16 access (wrong); a constant cast compiles
    (default flags) to a 3-instruction unsigned base materialization
    (`lui 0x15; ori 0xED48; ld 0(base)` — EGC splits 0x15ED48 as 0x150000 +
    0xED48 because the store keeps the base live, a named `.data` symbol
    compiles to a two-register signed load). Only a named `.data` extern
    under `-mno-split-addresses` emits the original self-based signed
    split: `lui a0,0x16; ld a0,-4792(a0)` (store gets its own `lui at,0x16;
    sd a0,-4792(at)`).
  - 0x10000800: EGC fuses every direct form — constant cast, named `.data`
    symbol, array/struct subscript, `&sym` pointer — into one pseudo load
    (`lwu $r, 0x10000800` / `lwu $r, sym+2048`) that the assembler expands
    self-based and fused (`lui a1,0x1000; lwu a1,2048(a1)`), dropping the
    explicit zero-extend. The original instead keeps the address as a live
    value (`lui a1,0x1000; ori a1,a1,0x800`, scheduled between the two
    global loads) with a separate `lw v1,0(a1)` plus `dsll32/dsrl32`
    zero-extend for the 64-bit add. The ONLY form that reproduces this is a
    **volatile constant-address cast**: `*(volatile unsigned int*)0x10000800`
    (probe v8, 64/64). Volatile suppresses the pseudo-load fusion; EGC
    materializes the address with `li` (value split -> `lui/ori`) and emits a
    real signed `lw` + explicit 32->64 zero-extend. A named volatile symbol
    (`extern volatile u32`) still fuses (`lw $r, D_0010000800` -> `lui/lw`
    self-based, probe v9, 60 bytes). Because the 0x10000000 region is
    EE-hardware-mapped with no ELF symbol, the literal stays a named define
    (`VU0_TICK_VALUE_ADDR`) with the constraint documented in the source.
- Flags: `permcb.o` needs `PRIVATE_COMPILE_FLAGS = -mno-split-addresses`
  (Makefile). permcb.cpp contains only this function, so the flag is local.
- Probe history (v8 = matched, v9 = named-volatile contrast, kept in
  decomp_state/probes/; v1-v7 discarded): v1 named .data + -mno-split, wrong
  constant address (0x1000800 misread of Splat comment) — 52 B; v2 constant
  casts, default flags — 52 B (3-instruction base for RMW, fused lwu);
  v3-v5 named symbols, default/-mno-split — 48/52 B (in-window form fixed
  only by -mno-split; 0x10000800 still fused); v6 pointer variable,
  -mno-split — 52 B (EGC CSEs the pointer into a fused pseudo); v7 array
  subscript over a named 0x10000000 base, -mno-split — 52 B (EGC folds
  sym+index into a pseudo); **v8 volatile constant cast, -mno-split — 64/64
  MATCH**; v9 named volatile symbol, -mno-split — 60 B (fused).
- Verification: probe v8 `match: true` (64/64 bytes, word-for-word);
  `make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf` byte-for-byte;
  decomp count 711 -> 710.
