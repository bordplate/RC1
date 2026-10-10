# func_00200600 (vram 0x200600, 0x354 bytes = 213 words) - BLOCKED

`code/game/hud_post_post2_post2.cpp:15`. HUD rotated-quad VU1 command-chain builder.

## Semantics
`void func(u32 a0, u32 a1, u64 a2, f32 f12, f32 f13, f32 f14, f32 f15, f32 f16)`.
Builds a rotated quad (two half-axes rotated by `f16`) around a center, then appends
5 VU1 command packets to `vu1ChainHead`. Callers pass a0=a1=0x40, a2 = low word of the
64-bit texture handle from `func_001FFA10` (0x1FFA10), f12=4096, f13=(f32)(n*0x10),
f14=f15=272, f16 = angle. Note 5 float args (f12-f16), non-standard O32; the caller
sets all five in registers.

## SOLVED portion (FP prologue + vector ops) - 0 diffs at 0x20064C..0x200724
KEY INSIGHT (delay-slot semantics, from last-resort-decompiler GPT-5.6 Sol): the
`swc1 $f0, 0x0($sp)` at 0x200660 is the DELAY SLOT of `jal FastCos` and executes
BEFORE FastCos runs, so it stores `B*sin` (the CALL1 result), NOT cos. Nothing is
discarded. The apparent "discarded B*sin" was a misread of the delay slot.

The sin,cos,cos,sin call order comes from the natural source:
```cpp
v0.x = fd * FastSin(fe);   // sin call 1 -> sp+0x0 (delay slot of cos#2)
v0.y = fd * FastCos(fe);   // cos call 2 -> sp+0x4 (delay slot of cos#3)
v1.x = -fc * FastCos(fe);  // cos call 3 -> sp+0x10 (delay slot of sin#4)
v1.y = fc * FastSin(fe);   // sin call 4 -> sp+0x14 (delay slot of jal FastVecAdd)
```
(An initial attempt with the swapped order v0.x=cos / v0.y=fd*cos did NOT match.)

Vector ops (16-byte stack vec4s), all matched:
```cpp
v2 = (f12, f13, 0, 0);
FastVecAdd(&t, &v2, &v0); FastVecSub(&v3, &t, &v1);  // (v2+v0)-v1
FastVecAdd(&t, &v2, &v0); FastVecAdd(&v4, &t, &v1);  // (v2+v0)+v1
FastVecSub(&t, &v2, &v0); FastVecSub(&v5, &t, &v1);  // (v2-v0)-v1
FastVecSub(&t, &v2, &v0); FastVecAdd(&v6, &t, &v1);  // (v2-v0)+v1
```

## THE WALL (VU1 packet section 0x200728.., ~124 bytes)
The original EGC builds the 64-bit VU1 packet constants with explicit `ori/dsll/dsll32`
sequences, e.g. `0xB400000000008001` via:
```
ori $11, $0, 0xB400
dsll32 $11, $11, 16
ori $11, $11, 0x8001
```
Local EGC 2.95.2 emits the `dli` 64-bit pseudo for the same value, which ps2eeas
expands to DIFFERENT bytes. The probe compares FINAL assembled bytes, so `dli` !=
`ori/dsll`. This is compounded by:
- Interleaved scheduling: the original interleaves constant construction with the
  `sw` stores of base[0..3] and does 5 volatile re-loads of vu1ChainHead + 1 advance
  store; the candidate loads the base once and does stores-then-constants.
- Size difference: candidate 868 bytes vs original 852 (16 bytes).

Tried (all via tools/decomp_probe.py, default TU flags, --define vu1ChainHeadStore):
- p3: corrected FP prologue -> 138 diffs, first at 0x200728 (FP+vector match).
- p4: 64-bit `u64* p=(u64*)base` packet stores -> 140 diffs.
- p5: explicit-op constant `p[2]=(u64)0xB400<<48|0x8001` -> 140 diffs (EGC
  constant-folds back to dli).

## Packet layout (for a future re-attempt)
base = initial vu1ChainHead:
- base[0..3] (sw): 0x10000007, 0, 0, 0x50000007; then head = base+0x10
- base+0x10.. (8 x sd): 0xB400000000008001, (0xA6A6A6A6<<11)|0x106, a2, 0x154,
  0x807F7F7F, a0<<4, vertex(v3), (a1<<20)+(a0<<4)
- base+0x50 (sd): vertex(v4), 0 ; base+0x60 (sd): vertex(v5), a1<<20
- base+0x70 (sd): vertex(v6), 0 ; final head = base+0x80
- vertex(v) = ((s32)v.x+ovp.minX-8) | (((s32)v.y+ovp.minY-8)<<16) | (u32)hudHeap.vuField_0C
  (ovp=occlViewParams 0x13E500: minX+0x10, minY+0x14; hudHeap 0x19A3E8: vuField_0C+0x0C)

## Outcome
Retain INCLUDE_ASM. FP prologue + vector ops are solved and documented (re-attempt
start point). The VU 64-bit-constant codegen is the wall. Full-ELF parity preserved
(no source change).
