# func_001F5808 (draw_post_post) — DrawOcclSprite2

Address 0x1F5808, size 0x2A8 (680 bytes). Blocked 2026-10-01. Retained as
INCLUDE_ASM; full-ELF parity preserved.

## Semantics

Draws a textured "occlusion sprite" by appending a 0x90-byte (144-byte) VU1
packet to `vu1ChainHead` (0x160F00). It is the occlusion-sprite sibling of
`DrawSprite` (func_001F55D8, 0x80-byte packet) and shares its 10-argument
signature. The four draw coordinates are derived from the float rect (x, y, w,
h), clamped against the occlusion view window `occlViewParams` (0x13E500), and
the four corner/UV/texture values are packed into 64-bit qwords.

## Corrected signature (two independent u64 args)

```c
void func_001F5808(float x, float y, float w, float h,
                   s32 txU, s32 txV, s32 txW, s32 txH,
                   u64 color, u64 tex);
```

Probes used the cfront mangled symbol `DrawOcclSpriteN__FffffiiiiUlUl`.
Floats arrive in f12..f15 (f12=x, f13=y, f14=w, f15=h); the four int args in
a0..a3; **color in t0 and tex in t1 as two separate 64-bit register args**
(NOT one u64 in t0:t1). The caller FUN_001f6638 confirms this: `move t0,s2`
(color) before `jal 0x1f5808`, `move t1,s4` (tex) in the jal delay slot.

## Coordinate RTL (all four offset adds are LIVE)

```c
left   = trunc(x * 16.0f) + occlViewParams.minX - 8;   // minX at +0x10
right  = trunc((x + w) * 16.0f) + occlViewParams.minX - 8;
top    = trunc(y * 16.0f) + occlViewParams.minY - 8;   // minY at +0x14
bottom = trunc((y + h) * 16.0f) + occlViewParams.minY - 8;
if (left   > 0x9000) return;
if (right  < 0x7000) return;
if (top    > 0x9000) return;
if (bottom < 0x7000) return;
```

`func_001FA6D0` is the trunc(f) helper. 16.0f = 0x41800000.

**Delay-slot correction (the central earlier mis-trace):** the four
`addiu sX, v0, -8` are each in the *delay slot of the following* `jal`, so each
captures the *previous* call's returned offset value. All four adds are live;
there is NO dead-add / DCE wall here (the old "dead add" blocker was a
delay-slot mis-trace and was refuted by last-resort-decompiler). Original
register homes: left=s5($21), right=s4($20), top=s3($19), bottom=t0($8);
color also lands in t0 (spilled `sd t0,0(sp)`, reloaded `ld a0,0(sp)` before
the tail), tex in s8($24) via the call-1 delay slot `move s8,t1`.

## 0x90-byte packet layout (H = head at entry)

Header (4 u32, H+0..15): `0x10000008, 0, 0, 0x50000008`.
Then head -> H+16. 16-byte `lq/sq` copy of the `CameraQuad` template
`D_00160860` at H+16..31. Then head -> H+32. Then a 14-qword payload at
H+32..143, stored in this source order:

```
tex, big, 0x154, color, uv0, vert0, uv1, vert1, uv2, vert2, uv3, vert3, 5, 0
```

Final head -> H+144.

- `big   = (txU<<4) | ((txU+txW)<<14) | 0xA | (txV<<24) | ((txV+txH)<<22)`
- `uv0   = (txV<<20) | (txU<<4)`
- `uv1   = (txV<<20) | ((txU+txW)<<4)`
- `uv2   = ((txV+txH)<<20) | (txU<<4)`
- `uv3   = ((txV+txH)<<20) | ((txU+txW)<<4)`
- `mask  = 0x00FFFFF000000000UL`  (verified: 0xFFFF<<16 | 0xF000<<24)
- `vert0 = left  | (top<<16)    | mask`
- `vert1 = right | (top<<16)    | mask`
- `vert2 = left  | (bottom<<16) | mask`
- `vert3 = right | (bottom<<16) | mask`

`D_00160860` is the 16-byte template (CameraQuad, mode(TI) -> lq/sq); it is in
the lit/gp-window region like D_00160840 (the DrawSprite template) and is not
yet in symbols.txt (probed with `--define D_00160860=0x160860`).

## Candidate matrix (all via tools/decomp_probe.py, production default flags
`-G8 -O2 -ffast-math -fno-exceptions -snas`)

| cand | form | size | in-range diffs |
|------|------|------|----------------|
| cand2 | q[] head-increment, no pins | 696 | 120 |
| cand3 | q[] + coord pins (s5/s4/s3/t0) | 684 | **101 (best size)** |
| cand4 | q[] + coord + UV/vert value pins | 696 | 104 |
| cand5 | q[] + coord + intermediate-temp pins | 684 | 101 |
| cand6 | fixed base register, coord pins | 620 | 94 (best diff, 60B short) |

Flag sweep on cand3: `-fno-schedule-insns` 156, `-fno-schedule-insns2` 130,
`-fno-gcse` 101, `-mno-split-addresses` 114 — all worse or equal. Default is
best.

## The wall (stable head-advance interleaving + tail RA)

With the coord pins, the **prologue and all four guards match byte-for-byte**
(the arg moves come out s7=a3/s6=a2/s2=a1/s1=a0/s8=t1 exactly like the
original; the only guard-region residual is four `bnez`/`bnezl` offsets shifted
by the 4-byte size delta). The entire residual is the tail (0x1F5918..0x1F5AAC,
~91-97 words), and it is a **scheduling + register-allocation wall**, not an
RTL error — the candidate emits the same operations in a different order with
different register homes:

1. **Head-advance interleaving.** The original keeps the entry head in a FIXED
   base register (t4) and advances the global in three interleaved steps:
   store the 4 header u32s, then head->H+16 (`addiu v1,t4,16; lui at; sw`),
   store the 16-byte template, then head->H+32 (`addiu v1,t4,32; ...`), the
   tex `sd s8,32(t4)` and the 13 remaining payload `sd`s relative to v1=H+32,
   then a final `lw head; addiu head,112; sw` (H+32+112 = H+144). No source
   form reproduces this exact store/advance interleaving:
   - the q[] head-increment idiom (cand2/3/4/5) increments the pointer
     (`head+=16; head+=16; ...`) and collapses the two mid-advances, landing
     4 bytes long (684B);
   - the fixed-base idiom (cand6) hoists all four header stores onto the base
     register before advancing, landing 60 bytes short (620B) and reordering
     the template `lq/sq` and the `0xA`/tag2 constant setup.
2. **Tail RA.** The original keeps every coord/arg live in an s-register
   (left=s5, right=s4, top=s3, bottom=t0, txU=s1, txV=s2, txW=s6, txH=s7,
   tex=s8) and packs the UV/vert temps into t0..t7/a0..a3, even computing
   `txU<<4` redundantly into BOTH s0 and a1. EGC instead reuses the coord
   registers destructively (e.g. `dsll s3,s3,16` overwriting top for
   top<<16), moves txU+txW to t1 and txU<<4 to t4, and holds the tag
   (0x50000008) in t2 rather than a1. The UV/vert and intermediate-temp
   register pins (cand4/cand5) do not move the tail — EGC's allocator keeps
   its own homes for the 64-bit values.

This is the same residual class that blocked the four siblings
(`DoGifPaging__Fv` 0x1F4398/16, `DrawRectOverlay_FiiiiUl` 0x1F52A0,
`DrawTexturedQuad` 0x1F5450, and `func_001F55D8` DrawSprite 0x230 best-65-diff
after 17 candidates — see notes/draw_post_func_001F55D8.md), on a 0x90-byte
packet / 680-byte body that is larger than DrawSprite's.

## Escalation

`last-resort-decompiler` (GPT-5.6 Sol, task ses_f0ab061c0ffeYqDqeCw8fzoFVZ,
invoked 2026-10-01) refuted the earlier dead-add/DCE blocker as a delay-slot
mis-trace, supplied the corrected ten-argument / two-u64 / four-live-coordinate
RTL and the full 0x90-byte payload reconstruction above, and stated that a
durable blocker requires mechanically testing the corrected form and then
demonstrating stable RA/scheduler residual clusters analogous to the siblings.
That was done (cand2-cand6 + flag sweep): the prologue/guards match and the
tail is a stable head-advance-interleaving + RA wall. No concrete new source
form, pin, or flag was recommended beyond the corrected RTL already tested.

## Re-attempt start point

cand3.cpp (q[] head-increment + the four coord pins, 684B / 101 diffs) is the
closest in size; cand6.cpp (fixed base, 620B / 94 diffs) is closest in diff
count. A future crack needs the original's three-way store/advance interleaving
with the head kept in a fixed t4 base — the sibling DrawSprite note and
notes/draw_post_DoGifPaging__Fv.md describe the same tie-break class. Probe
with `--define D_00160860=0x160860` and mangled symbol
`DrawOcclSpriteN__FffffiiiiUlUl`; the final source must emit the exact linker
symbol `func_001F5808` (extern "C" / asm label), and `D_00160860` should be
added to config/symbols.txt.
