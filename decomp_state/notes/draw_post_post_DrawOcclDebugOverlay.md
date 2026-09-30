# DrawOcclDebugOverlay (func_001F5138, 0x1F5138, 0xD8) — MATCHED 2026-09-30

`code/game/draw_post_post.cpp`. The occlusion-debug overlay renderer, the
occlusion twin of the matched sibling `DrawScreenEffect` (0x1F4FB8). Unlike
that sibling it takes the debug overlay's `ScreenVBEffect*` config as a
parameter (the block based at `occlDebugOverlayEnabled`, 0x15F370) instead of
reading the global `screenColorEffectNow`, and it draws only the background —
no A/B effect bands.

Caller: `DrawDebugProfiler` (0x1F39D0, still INCLUDE_ASM) at 0x1F3BC0 passes
`0x15F370` in a0. Because the caller is hand-written assembly that references
the target by its Splat placeholder name, the definition keeps that symbol via
a declaration asm-label (`void DrawOcclDebugOverlay(ScreenVBEffect*)
asm("func_001F5138");`) — the old cfront rejects the label on the
definition-with-body, so it goes on a separate declaration (same pattern as
`AddDrawCallback2` / `func_001F47B8`).

## Behaviour

```cpp
void DrawOcclDebugOverlay(ScreenVBEffect* effect) {
    if (effect->bkgAlpha != 0)                       // u64 at +0x08
        VU1_addGSregister(VU1_SCREEN_ALPHA_GS_REG,   // 0x42
                          effect->bkgAlpha & SCREEN_EFFECT_ALPHA_MASK);
    if ((effect->bkgColor & SCREEN_EFFECT_COLOR_ENABLE) != 0) {   // u32 +0x04
        VU1_addGSregister(OCCL_DEBUG_GS_REG,         // 0x4E
            (int)frameBufferBase >> 13 | OCCL_DEBUG_BASE_COMMON | OCCL_DEBUG_BASE_LEAD);
        DrawRectOverlay(0, (s16)occlCamParamBase.drawH,
                        0, (s16)occlCamParamBase.drawW,
                        (unsigned long)effect->bkgColor);
        VU1_addGSregister(OCCL_DEBUG_GS_REG,
            (int)frameBufferBase >> 13 | OCCL_DEBUG_BASE_COMMON);
    }
    if (effect->bkgAlpha != 0)
        VU1_addGSregister(VU1_SCREEN_ALPHA_GS_REG, OCCL_DEBUG_END_ALPHA);
}
```

- `frameBufferBase` (0x15EE88, a u32 VALUE holding the base address) is read
  with a plain `extern u32` (no `.extern` seed → self-based `lui/lw` under the
  default `-G8` SN TU) and shifted `>> 13`.
- Constants (all hardware GS values, not data addresses):
  `OCCL_DEBUG_GS_REG 0x4E`, `OCCL_DEBUG_BASE_COMMON 0x1000000` (both writes),
  `OCCL_DEBUG_BASE_LEAD 0x100000000` (leading write only),
  `OCCL_DEBUG_END_ALPHA 0x8000000044` (trailing alpha write).
  `0x100000000` lowers to `li v0,0x8000; dsll v0,17`; `0x8000000044` to
  `li a1,0x8000; dsll a1,24; ori a1,0x44`.

## Codegen walls and solutions

1. **sra vs srl.** The original shifts `frameBufferBase` with a 32-bit
   arithmetic shift (`sra`). A `u32 >> 13` lowers to `srl` (logical); casting
   to `int` first (`(int)frameBufferBase >> 13`) yields `sra`. The whole OR
   expression stays 32-bit signed, so the `unsigned long` call argument is a
   sign-extension (no `dsll32/dsrl32` zero-extend pair). `(long)` casts were
   tried and rejected: they drop the zero-extend but widen the shift to `dsra`.

2. **Register map.** The original keeps the config pointer in s1 and the
   `0x1000000` base in s0. An early attempt pinned the constant to s0
   (`register int overlayBase asm("$16")`), which forced the prologue to match
   but broke the final OR (see 3). Pointer register pins are ignored by this
   EGC; only the prologue constraint mattered, and the bare constant
   (solution 3) reproduces the map without a pin.

3. **The decisive OR operand order.** The rectangle-following GS write's value
   `(fb>>13) | 0x1000000` is ORed in the `jal` delay slot. The original emits
   `or a1,s0,a1` (base register first); the pinned-register form emitted
   `or a1,a1,s0` (in-place register first). Reversing the C operand order did
   NOT flip it — EGC normalizes a pinned in-place OR. The fix is to keep the
   `0x1000000` base a **bare constant** (no `register` pin): EGC then allocates
   it to s0 (prologue unchanged) but schedules the single-OR call as
   `or a1,s0,a1`. Probed in `working/func_001F5138/probe6.c`: variant A (pinned)
   → `or a1,a1,s0`; variants B/C (bare literal) → `or a1,s0,a1`, both keeping
   the two preceding ORs (`or a1,a1,s0`, `or a1,a1,v0`) in their original order.

## Verification

Standalone object matches; full `make` + `cmp build/boot_elf.elf
assets/boot_elf.elf` passes byte-for-byte. `decomp_status` count 613 → 612.
