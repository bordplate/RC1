# setupEffectDrawBuffer (0x1F7888, 236 bytes) — MATCHED

`void setupEffectDrawBuffer(int log2Width, int log2Height, int inPlace, float
xratio)` in `code/game/draw_post_post.cpp`. The symbol is kept as
`func_001F7888` via an `asm` label because the sole caller (func_002196B8,
pause_post.cpp, still INCLUDE_ASM) references `func_001F7888`.

## Semantics

Prepares the occlusion screen-effect draw buffer and view for one effect pass.

1. **GS base address** (`base`, in GS address units = VRAM >> 13):
   - `inPlace != 0` (main buffer): `base = occlCamParamBase.effectBufBaseGs`
     (+0x16E), the base recorded by SetupFS_AA_buffer.
   - `inPlace == 0` (scratch buffer): `t = log2Width + log2Height`,
     clamped `t = min(t, 0x10)`, then `base = (textureMemoryBase - (4 << t))
     >> 13` — a scratch region just below the texture pool.
2. **func_001FB440(log2Width, log2Height, base << 13):** builds the GS state
   packet for the effect buffer, packing `base` (shifted back to VRAM units)
   into the VU1 GS registers.
3. **func_001F33B8(1 << log2Width, 1 << log2Height, xratio, 0.0f, 524288.0f,
   255.0f, 0.0f):** sets up the view context — `occlViewParams` rectangle plus
   `viewCtx.xratio` and the fog defaults. The four floats are exactly the
   `InitViewContext` defaults: fogNearDist=0.0, fogFarDist=524288.0,
   fogNearIntensity=255.0 (clear), fogFarIntensity=0.0.
4. **VU1_addGSregister(0x47, inPlace ? 0 : 0x30000):** select the effect buffer
   source (main vs scratch).
5. **VU1_addGSregister(VU1_SCREEN_ALPHA_GS_REG, OCCL_DEBUG_END_ALPHA):** GS
   register 0x42 = 0x8000000044 (the occlusion debug end alpha).

Caller: func_002196B8 (pause_post.cpp, 0x219AE8), a loop that calls
`setupEffectDrawBuffer(log2Width, log2Height, inPlace, 1.0f)` — i.e. this
caller overrides the default xratio (0.63) with 1.0.

## Match-critical details

### The clamp is `min(t, 0x10)`, written with a pinned below-limit flag
The original computes `slti $a0, v1, 17; movz v1, a2, a0` (if `t >= 0x11` set
`t = 0x10`). The matching C is:
```cpp
register int limit asm("$6") = 0x10;
register int belowLimit asm("$4") = base < 0x11;
asm volatile("" : : "r"(belowLimit));
...
if (!belowLimit)
    base = limit;
```
`belowLimit` pinned to `$4` (a0) and `limit` to `$6` (a2) reproduce the
`slti a0, v1, 17` / `movz v1, a2, a0` pair.

### Register pins + input-only barriers
EGC 2.95.2's default scheduler/allocator needs explicit pins to reproduce the
original's zero-path register map and scheduling. The pins are:
`base`→$3 (v1), `limit`→$6 (a2), `belowLimit`→$4 (a0), `four`→$2 (v0),
`textureBase`→$5 (a1), `difference`→$6 (a2), `gsBase`→$6 (a2). Three INPUT-ONLY
barriers (`asm volatile("" : : "r"(x))`) anchor `belowLimit`, `four`, and
`gsBase` to fix scheduling without breaking EGC's if-conversion of the clamp.

Pitfalls (all mechanically tested, see `working/draw_post_post_func_001F7888/`):
- Tied read/write barriers (`"+r"(x)`) are catastrophic — they convert the
  `slti/movz` clamp into a branch and blow the size up to 240+ B (46-48 diff
  words).
- `-fno-schedule-insns` / `-fno-schedule-insns2` make it worse (17/19 words).
- An anchor-storm (pinning every temporary) over-constrains and regresses.
- The minimal pin set + three input-only anchors gives 0 diffs.

### gsBase must be `int`, not `long`
`gsBase = base << 13` emits a 32-bit `sll a2, v1, 0xd` (funct 0). Casting to
`long` emits a 64-bit `dsll`, which does not match. The `gsBase` anchor keeps
the `sll` in the body (ahead of the `move a0`/`move a1` arg setup) rather than
in the `jal` delay slot.

### The float is a local
`float x = xratio;` keeps the incoming f12 (xratio) in a saved local (f20),
reproducing the prologue's `lqc $f20` / `sqc1 $f20` save-and-reload. Passing the
parameter directly changes the FPU save structure.

## Integration notes

### cfront cannot take an `asm` label on a function definition
`void setupEffectDrawBuffer(...) asm("func_001F7888") { ... }` is a parse error
(`parse error before '{'`) in EGC 2.95.2 cfront. The label must live on a
separate declaration; the plain definition inherits the symbol:
```cpp
void setupEffectDrawBuffer(int, int, int, float) asm("func_001F7888");
void setupEffectDrawBuffer(int log2Width, int log2Height, int inPlace, float xratio) { ... }
```
This emits the global symbol `func_001F7888` (verified with `nm`).

### Callee declarations
`func_001F33B8` (same TU, INCLUDE_ASM at draw_post_post.cpp:313) and
`func_001FB440` (framebuf.cpp:24) are still placeholders; forward declarations
with `asm("func_...")` labels bind the calls to the unmangled symbols the
generated assembly defines.

## Layout added to camera.h
`struct OcclCamParamBlock` (0x151780) extended through +0x170: pads at
0x15C-0x16C and `s16 effectBufBaseGs` (0x16E) — the current effect buffer base
in GS address units — plus `u32 pad_170`. Tail-extension only; existing
matchers (drawW/drawH at 0x150/0x152) are unaffected.

draw_post_post.o matches all 236 bytes; full boot ELF parity passes (clean
`make clean && make split && make -j2` + `cmp`).
