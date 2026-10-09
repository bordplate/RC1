# Transition_DrawSky__Fv (0x1E9AB8, 0x58) — matched 2026-10-09

## Semantics
Draws the sky level. A short wrapper that, in order:
1. `SetupSkyGifPaging()` (skyfunc.cpp, 0x22B4C8) — set up the sky GIF pages.
2. `SkyLevelGeneric()` (skyfunc.cpp, 0x22AE70) — draw the generic sky content.
3. `DoSkyGifPaging()` (skyfunc.cpp, 0x22B558) — run the sky GIF paging.
4. `VU1_addGSregister(0x47, 0x5360b)` — program GS register 0x47 with a fixed
   value (the sky texture setup; the Deadlocked descendant calls the identical
   `(0x47, 0x5360b)` around textured-quad draws in transitionfunc/mobyfunc/freeze).
5. `VU1_addGSregister(0x4e, (int)frameBufferBase >> 13 | 0x1000000)` — program
   GS register 0x4e with the frame-buffer-derived base. This is the SAME register
   and value form the occlusion overlay uses (`OCCL_DEBUG_GS_REG`/`OCCL_DEBUG_FB_SHIFT`/
   `OCCL_DEBUG_BASE_COMMON` in draw_post_post.cpp `DrawOcclDebugOverlay`).

void return; the sole caller is the transition draw path. The three sky callees are
still `INCLUDE_ASM` in skyfunc.cpp; `VU1_addGSregister` is matched in draw_post_post.cpp.

## Declarations
- `SetupSkyGifPaging`/`DoSkyGifPaging` mangle naturally to `...__Fv` (no asm override).
- `SkyLevelGeneric` is referenced via `asm("SkyLevelGeneric___maybe")` — the ELF is
  stripped so the real mangled name is unknown and the existing symbols.txt name is
  retained (the callee is still an `INCLUDE_ASM` placeholder).
- `VU1_addGSregister(unsigned int reg, unsigned long value)` with the
  `asm("VU1_addGSregister__FUiUlb")` override (same 2-param form as draw_post_post.cpp;
  its 3rd/4th params are never materialized at RC1 call sites).
- `frameBufferBase` must be declared `extern unsigned int ... __attribute__((section(".data")))`
  in this TU: with `-mno-split-addresses` a plain in-window extern lowers to a
  GP-relative load, but the original reads it with a self-based absolute `lui/lw`.
  (Spelled `unsigned int`, not `u32`: this EGC/cfront build rejects the `u32` typedef
  immediately before `__attribute__` on a scalar extern here, though camera.cpp:9 uses it
  under different flags.)

## Match details
22-instruction wrapper. The prologue is a plain 0x10 frame saving only `$ra`. The first
three callees are no-arg `jal`/`nop` pairs. The two `VU1_addGSregister` calls set `a0`
(the reg, a small constant) in the `jal` delay slot and `a1` (the value) before it.

The value for the 0x4e write is `0x1000000 | ((int)frameBufferBase >> 13)`. The signed
`int` cast is required: a `u32` shift would lower to `srl`, but the original uses
`sra` (`00052b43`). The 0x1000000 constant materializes as `lui v0,0x100` (0x100 << 16
= 0x1000000; Splat prints the raw bytes `00 01 02 3C` and its `(0x1000000 >> 16)` comment
is misleading — decode the raw word). Under `-mno-split-addresses` EGC emits the value as
a single `li $2,16777216` pseudo which ps2eeas expands to that `lui`.

Named GS register constants (`SKY_SETUP_GS_REG`=0x47, `SKY_SETUP_GS_VALUE`=0x5360b,
`SKY_OVERLAY_GS_REG`=0x4e, `SKY_OVERLAY_FB_SHIFT`=13, `SKY_OVERLAY_VRAM_BASE`=0x1000000)
are local `#define`s (textual, no codegen change) — the register offsets are hardware-
defined and unnamed in the available references, same treatment as the existing
`VU1_*_GS_REG` / `OCCL_DEBUG_*` constants.

## Flag
`game/transition.o` receives `PRIVATE_COMPILE_FLAGS = -fno-schedule-insns -mno-split-addresses`
(unchanged; the matched Help_LoadMsgs in the same TU still passes).

## Verification
Built `transition.o` `Transition_DrawSky__Fv` (22 instrs at offset 0) matches the original
word-for-word (only the `jal`/`lw` fields are relocations, resolved at link). Clean
`make` plus `cmp build/boot_elf.elf assets/boot_elf.elf` passes (byte-identical).
Target count 554 -> 553.
