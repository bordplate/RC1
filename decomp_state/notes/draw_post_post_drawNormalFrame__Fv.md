# drawNormalFrame__Fv (0x1F4248, 0x34 = 13 words)

Per-frame draw setup for the normal draw states (caller state 0/8, the
non-space-load path). Renamed from the Splat placeholder `func_001F4248`.

```
if (spaceLoadInProgress) return;
framebuf_appendLargeSetup();
drawEnableMask = DRAW_ENABLE_MASK_BASE;   // 0x7F
DrawDebugProfiler();
```

Matched byte-for-byte on the first candidate; full clean-boot parity holds.

## Match-critical details
- **Mixed address modes in one function.** `spaceLoadInProgress` (0x15F618)
  is loaded self-based absolute (`lui/lw`) — a plain `extern int` in this
  default-`-G8` TU gives that via EGC's bare-pseudo expansion. `drawEnableMask`
  (0x15F434) is stored **GPREL16** (`sw off gp`) in the `DrawDebugProfiler`
  call delay slot — reproduced with an `.extern`-seeded declaration
  (`asm(".extern drawEnableMask, 4");`), the same pattern as `partClipDistGp`
  in this file. A plain declaration would expand self-based and miss.
- **`DrawDebugProfiler` is C linkage.** Its generated `INCLUDE_ASM` supplies the
  unmangled symbol `DrawDebugProfiler`; calling it from this C++ TU requires
  `extern "C"` (a C++ declaration mangles the call to `DrawDebugProfiler__Fv`
  and fails the link).
- **Delay-slot scheduling is natural.** EGC places `sq $ra` in the `bnez` delay
  slot and the `drawEnableMask` store in the trailing `jal DrawDebugProfiler`
  delay slot with the plain source order (no barriers needed).

## Data
- `drawEnableMask` = 0x0015F434: bitmask of per-frame draw stages/features read
  by the main draw pass (`DrawDebugProfiler`, 0x1F39D0); bit N enables the Nth
  stage. The normal path resets it to `0x7F` (bits 0-6). Neighbors:
  `drawFrameCount` (0x15F438), `screenFade` (0x15F43C).
