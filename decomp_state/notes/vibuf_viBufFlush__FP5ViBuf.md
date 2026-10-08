# viBufFlush__FP5ViBuf (code/game/movie/vibuf.cpp)

- Original: 0x54 (84) bytes at vram `0x0023C660` (file offset `0x13C660`),
  C++ mangled symbol `viBufFlush__FP5ViBuf` (`void viBufFlush(ViBuf*)`).
- Semantics: rounds `self->field_0x14` (bytes put) UP to the next 0x800 VU
  block. Under the sema:
  `rounded = field + (VIBUF_BLOCK_SIZE - 1); alt = field + (2*VIBUF_BLOCK_SIZE - 2);
  if (rounded >= 0) field = round_down_0x800(rounded); else field =
  round_down_0x800(alt);` For non-negative field this is a round-up to 0x800;
  the second path covers the signed wrap case. The block size/shift are named
  VIBUF_BLOCK_SIZE (0x800) / VIBUF_BLOCK_SHIFT (11) in code/include/vibuf.h
  (added here; the pre-existing raw `<< 11`/`>> 11` in viBufCount and
  getFIFOindex are registered in refactor.json).
- Sema callees: first call = `WaitSema` (0x1189B0, syscall 68), second =
  `SignalSema` (0x118990, syscall 66) — confirmed against the matched
  siblings viBufEndPut/viBufCount, which use the same two entry points. Both
  are already `extern "C"` in vibuf.cpp; no new symbols/prototypes needed
  (all fields already named in code/include/vibuf.h: sema=0x40, field_0x14=0x14).
- Codegen: straight-line, two calls, no branches. EGC keeps `self` in s0,
  preloads `rounded` (a1) and `alt` (v1), materializes -1 into v0, and does a
  `slt v0,-1,rounded` + `movn v1,a1,v0` select; the `sw` of the result lands in
  the `SignalSema` jal delay slot. Frame 0x20.
- The one tie-break: a bare `if (rounded > -1) alt = rounded;` (or `(-1 < rounded)`,
  `rounded >= 0`, `(rounded+1) > 0`, `const int limit=-1`) all normalize in EGC
  2.95.2 to `slt rounded,0` + `movz` (uses the $0 register, drops the -1
  materialization, 80 bytes / 14 word diffs). The original keeps the -1 as a
  live register and uses `movn`. Writing the -1 as an ordinary (non-const) local
  — `int limit = -1; if (limit < rounded) alt = rounded;` — defeats the
  comparison simplification and reproduces the original byte-for-byte. `const`
  folds back into the normalized form, so it must stay a plain local.
- Full make + `cmp build/boot_elf.elf assets/boot_elf.elf` passes (byte-for-byte).
