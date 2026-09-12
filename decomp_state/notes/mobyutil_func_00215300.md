# func_00215300 / func_00215348 (code/game/mobyutil.cpp)

Sibling "count active flags" counters, decompiled together (identical
structure, independent pools).

- Originals: 72 bytes (`0x48`) each, at vram `0x215300` and `0x215348`.
  Both are unmangled symbols: callers reference them directly with
  `jal func_00215300` / `jal func_00215348` (pause_post: DrawItemsMenu,
  DrawEndScreenMenuMaybe, func_00226E58; mobyutil: func_00215248).
- Semantics: each scans a pool of per-entity "is active" flag bytes and
  returns the number of nonzero (active) entries, clamped to a display
  maximum:
  ```cpp
  int func_00215300(void) {  // pool 0x13E520, 0x25 bytes, cap 0xA
      int i, n = 0;
      for (i = 0; i < 0x25; i++) if (D_0013E520[i]) n++;
      if (n < 0) n = 0;
      if (n > 0xA) n = 0xA;
      return n;
  }
  int func_00215348(void) {  // pool 0x13D408, 0x20 bytes, cap 0x1E
      int i, n = 0;
      for (i = 0; i < 0x20; i++) if (D_0013D408[i]) n++;
      if (n < 0) n = 0;
      if (n > 0x1E) n = 0x1E;
      return n;
  }
  ```
  The pools are zeroed in the boot image and populated at runtime. They are
  used by a debug/HUD display (pause_post) that shows `count / capacity`
  for each pool; the capacity (0xA / 0x1E) is the clamp.
- Naming: the specific entity domain of the pools could not be established
  from the boot image (the label strings are loaded from an overlay at
  runtime, so the string-table pointers at 0x15F6A0 are zero in the boot
  ELF). Per the style guide the Splat address names are retained for both
  the functions and the pools, with this note recording what is known.
- Linkage: the symbols are unmangled in the build (generated asm `glabel`
  and caller `jal`s all use `func_00215300`), and the original linkage
  cannot be confirmed from the stripped ELF, so the C++ definitions carry
  an `asm("func_00215300")` label (the codebase convention for unmangled
  address symbols) rather than an `extern "C"` assumption.
- Match: matched with the default mobyutil.o flags (`-G8 -O2`, no private
  flags). Verified against a standalone probe before editing. Key codegen
  points EGC reproduces exactly:
  - the loop precomputes `count+1` (`addiu a0,a2,1`) before the `lbu`, and
    the conditional increment is the `movn a2,a0,v1` in the `bnez` delay
    slot;
  - the pool must be declared `unsigned char` so the byte load is `lbu`
    (a signed `char` gives `lb` and a 1-word mismatch);
  - the clamp tail lowers to `li v1,-1; slt v1,v1,a2; movz a2,zero,v1`
    (max(n,0), a no-op for a nonnegative count) then
    `li v0,cap; slti a0,a2,cap+1; jr ra; <movn v0,a2,a0>` (min(n,cap)) with
    the final `movn` in the `jr` delay slot.
  - the pool address is out of the gp window (gp 0x166C00), so the
    `D_0013E520`/`D_0013D408` symbol references emit the signed
    `%hi/%lo` split (`lui 0x14; addiu -0x1AE0` / `-0x2BF8`), matching the
    original (a constant cast would give the wrong unsigned `ori` split).
- Verification: full `make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf`
  byte-for-byte; decomp count 708 -> 706.
