# effects func_001EDC50 (drawEffectRibbon) — BLOCKED

vram 0x1EDC50, 948 bytes / 237 instr, `drawEffectRibbon__Fv` (no params).
Single caller: DrawDebugProfiler (0x1F39D0), gated `if (D_0015F434 & 0x20)`.
Draws 16 textured quads ("effect ribbon") fanning out from the camera.

Status: **blocked** (EGC 2.95.2 register-allocation wall). Semantics, structs,
and ABI are fully reverse-engineered and correct; a candidate reproduces the
EXACT 948-byte size with `-fno-schedule-insns2` but cannot reproduce the
original's callee-saved register assignment. Best probe: 948 bytes, ~167
byte-diffs (all allocation/schedule, none semantic).

## Verified semantics

```
if (effectRibbon.state != 1 || (effectRibbon.head->flags & 0x80)) {
    effectRibbon.state = 0; return;
}
if (effectRibbon.trigger <= 0.0f) return;

dist   = FastVecDist(&currentCamera.pos /*0x187080*/, &head->pos /*head+0x10*/);
center[0] = func_001FA6C0(occlViewParams.halfWidth);   // int->float
center[1] = func_001FA6C0(occlViewParams.halfHeight);
projectWorldPoint(proj, &head->pos);
proj[0] = (proj[0] - func_001FA6C0(occlViewParams.left)) * 0.0625f;
proj[1] = (proj[1] - func_001FA6C0(occlViewParams.top))  * 0.0625f;
offset[k] = center[k] - proj[k];

for (i = 0; i < 16; i++) {
    scale = (i == 0) ? 64 : 32;
    tex   = GetEffectTex(texId[i] + 6, <stale a1>);   // callee IGNORES arg2
    ds    = distScale[i];                              // == size[i-16]
    scaled[k] = offset[k] * ds;
    screen[k] = center[k] + scaled[k];
    c  = clamp01(dist - 5.0f);                          // re-materialized each iter
    t0 = clamp01(FastAbsF(offset[0]) / center[0] * -5.0f + 5.5f);
    t1 = clamp01(FastAbsF(offset[1]) / center[1] * -5.0f + 5.5f);
    t  = t0 * t1;                                        // NO separate "sel" var
    alphaI = func_001FA6D0(c * t * (float)alphaMul[i]);  // float arg in f12
    big   = func_001FA6D0((float)scale * size[i]);
    half  = big >> 1;
    x0 = func_001FA6D0(screen[0] - (float)half);
    y0 = func_001FA6D0(screen[1] - (float)half);
    col     = color[i] & 0x00FFFFFF;
    packed  = (alphaI << 24) | col;
    DrawTexturedQuad(x0, y0, big, big, 0, 0, scale, scale, packed, tex);
}
effectRibbon.trigger = 0.0f;
```

## Verified struct layouts

```
struct EffectRibbonAnchor {           // head points here
    u32 pad_00[4];                    // 0x00
    f32 posX, posY, posZ;             // 0x10  (pos = head + 0x10)
    f32 field_1C;                     // 0x1C
    u8  flags;                        // 0x20  (bit 0x80 = active)  [NOT 0x23]
};
struct EffectRibbon {                 // @ 0x187300 (.data)
    EffectRibbonAnchor* head;         // 0x00
    u32 state;                        // 0x04
    f32 trigger;                      // 0x08
    u32 pad_0C;                       // 0x0C
    s16 alphaMul[16];                 // 0x10
    s16 texId[16];                    // 0x30
    u32 color[16];                    // 0x50
    f32 distScale[16];                // 0x90  (== size[i-16])
    f32 size[16];                     // 0xD0
};
struct OcclViewParams {               // @ 0x13E500 (core.data)
    u32 width, height, halfWidth /*+8*/, halfHeight /*+0xC*/,
        left /*+0x10*/, top /*+0x14*/, right, bottom;
};
```

## KEY findings

1. **GetEffectTex (0x1F44B8) NEVER reads a1** — the original leaves `a1` stale
   at the in-loop call (clobbered by projectWorldPoint). The 2nd arg is NOT a
   tracked live value. Match with a ONE-ARG prototype bound to the mangled
   symbol: `u64 GetEffectTex1(int) asm("GetEffectTex__Fii")` called as
   `GetEffectTex1(texId[i]+6)`. Passing `(int)pos` (2-arg) makes pos
   loop-live and adds a per-iter `move a1,s8` (worse).

2. **DrawTexturedQuad trailing params are u64, not u32** (last-resort GPT-5.6
   Sol finding). Original: `sd s0,0(sp); sd s7,8(sp)` (64-bit stores). A `u32`
   prototype makes EGC zero-extend (`dsll/dsra`) + `sw`, adding exactly the 8
   excess bytes (956->948). Correct: `extern "C" void DrawTexturedQuad(int x0,
   int y0, int w, int h, int a, int b, int c, int d, u64 packed, u64 tex)`;
   keep `tex` as `u64` (un-truncated GetEffectTex1 return) and `packed` as `int`
   ((alphaI<<24)|col), implicit-converted at the call.

3. `c = clamp01(dist - 5.0f)` — NO abs; 0x40A00000 = +5.0f, re-materialized
   every iteration (EGC quirk). `t = t0 * t1` directly (no `sel`; t1 is already
   clamped, a separate `sel==t1` block emits dead code).

4. Pre-loop call order: FastVecDist -> FA6C0(halfWidth) -> FA6C0(halfHeight)
   -> projectWorldPoint -> FA6C0(left) [in proj[0] expr] -> FA6C0(top) [in
   proj[1] expr]. center/offset must be unnamed float[2] pairs (named locals
   get promoted to saved FP regs and blow the frame to 0x1D0).

## Original register/stack plan (objdump)

- s0 = base (0x187300) via `addiu s0,v0,29440` (v0=lui 0x180000); s1 = lui.
  state `lw v1,4(s0)`; trigger `lwc1 f0,8(s0)`; head `lw a1,29440(s1)`; flags
  `lbu v0,32(a1)`. Then `move s8,s0` (s8=base for the loop) and s0 REUSED for
  occlViewParams (`lui s0,0x14; addiu s0,s0,-6912`).
- pos in **a1** (`addiu a1,a1,16`); orig RELOADS head before projectWorldPoint
  (not a persistent reg).
- Loop: s4=i(+=1); s3=scale (`li s3,64; movn s3,v0,s4`); s6=size ptr
  (base+0xD0, +=4); s7=tex u64; s5=base+16 (alphaMul base, materialized in
  delay slots).
- **Induction:** `sll v0,s4,1; move s0,v0` computes i*2 ONCE into s0, shared:
  texId `addu v0,s8,v0; lh a0,48(v0)`; alphaMul `addu v0,s0,s5; lh v1,0(v0)`.
  color via running byteOff(96sp): `lw v1,96(sp); addu a0,s8,v1; lw a1,80(a0)`.
  size is the ONLY pure running pointer (s6).
- FP consts: f20=0.0625, f21=c, **f22=0.0, f23=1.0**, f24=5.5, f25=-5.0,
  f26=dist. Frame 0x150: proj@16, center@32/36, offset@48/52, scaled@64/68,
  screen@80/84, byteOff@96, s0-s8@112-240, ra@256, f20-f26@272-320.

## Probe results (tools/decomp_probe.py)

| variant | formulation | bytes | diffs |
|---|---|---|---|
| probeE | pos cache, 1-arg tex, running ptrs, u32 ABI | 956 | 158 |
| probeI | head-reload, u64 ABI | 952 | 209 |
| probeJ | pos cache, u64 ABI | 952 | 198 |
| probeK | scoped s0->s8 base transfer | 924 | 229 (too short) |
| probeL | u64 + hard s0 index2 (texId/alphaMul) + volatile byteOff | **948** | 168 |
| probeM | probeL + full reg pins (s8 base,s3 scale,s6 size,s7 tex) | 944 | 175 (too short) |
| probeN | probeL + base pinned to s0 | 944 | 224 |
| probeL + -fno-schedule-insns2 | (best) | **948** | **167** |

Flag results: `-fno-schedule-insns` (956/230, worse), `-fno-strength-reduce`
(964/214, worse — orig needs size as a running ptr).

## Why blocked

probeL (best) reproduces the exact 948-byte size but the remaining 167 byte
diffs are ALL interdependent EGC 2.95.2 allocation choices, none semantic:
1. base lands in **s5** (orig s0->s8); EGC picks s5 for the single `&effectRibbon`
   local in every formulation tried. Pinning s0 (probeN) or s8 (probeM) or a
   scoped s0->s8 transfer (probeK) all change the size (944/944/924) and do NOT
   match — the original's dual s0/s8 base lifetime is not reproducible.
2. frame 0x140 (7 saved s-regs) vs orig 0x150 (9 saved s-regs: s1, s8 extra).
3. FP const regs / per-instruction scheduling differ across the whole body.

Escalation: `expert` (GPT-6 Astra) used (gave the 1-arg GetEffectTex insight ->
probeE). `last-resort-decompiler` (GPT-5.6 Sol) used (gave the u64 ABI fix,
applied, 956->948; Tests 3-6 = scoped base / hard index2 / volatile byteOff /
reg pins, all tried as probeK/L/M/N, none match). No further formulation is
available; the dual s0/s8 base register + 0x150 frame is an unmatchable
allocator decision for this EGC build.

## Durable side effects (kept)

- symbols.txt: `effectRibbon = 0x187300`, `occlViewParams = 0x13E500` (real
  data, verified layout), `projectWorldPoint__FPfT0 = 0x1f2070` (redundant —
  already defined in code/game/draw_post.cpp). Parity verified with these pins.
- Function name `drawEffectRibbon` (used only in probes; source stays
  INCLUDE_ASM, so the generated .s keeps the func_001EDC50 placeholder).
