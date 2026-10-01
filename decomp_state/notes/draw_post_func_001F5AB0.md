# func_001F5AB0 (0x1F5AB0, 0x460 = 280 words) — BLOCKED (RA/scheduler tie-break wall)

Target: `INCLUDE_ASM` in code/game/draw_post_post.cpp. Queue index 29.
Sibling of DrawSprite (0x1F55D8, blocked) and DrawOcclSprite2 (0x1F5808,
blocked). Best candidate preserved inline below (linked-ELF word ratio 0.8449).

## Identity
VU1 occlusion "effect sprite" appender. The effect renderer (func_001EE008,
blocked) draws its sprites into the occlusion grid through this, which appends
a 0x80-byte packet of four (uv, vert) corner pairs to the VU1 chain. Same
packet family as the siblings (header 0x10000007/0/0/0x50000007; +0x10 128-bit
template; +0x20 tex; +0x28 0x154; then uv/vert pairs). Called only by
func_001EE008 — 6 call sites. Call shape (from the effects.cpp caller
candidate):

```
func_001F5AB0(x, y, 40*scl, 40*scl, angle, z, z, 0x3F, 0x3F, texId, 0xFFF3,
              spr->data, flagU, flagV)
```
floats f12..f18 = x,y,w,h,angle,z1,z2 ; ints a0..a3,t0..t2 =
uBase,vBase,texId,colour,data,flagU,flagV. EGC 2.95.2 int-arg window is
a0,a1,a2,a3,t0,t1,t2,t3 (no a4/a5); floats f12-f18 consecutive.

## Prologue (ground-truth objdump)
frame 0x170. andi t1,t1,0xff / andi t2,t2,0xff = zero-extend => the flag
params are unsigned char (u8). s4 = a3 (colour); sd a2,128(sp) = texId 64-bit
(upper half = a3 = colour, deterministic garbage). f23=angle, f24=z2, f25=h,
f26=z1, f27=w. Flag logic (branch delay slots ALWAYS execute):

```
if (flag0) { uMain = uBase<<4; uFlip = 16; } else { uFlip = uBase<<4; uMain = 16; }
if (flag1) { vMain = vBase<<20; vFlip = 0x100000; } else { vFlip = vBase<<20; vMain = 0x100000; }
sw t0,136(sp)  -- data spill, UNCONDITIONAL (delay slot of beqz t1)
```

## Math section
Trig (4 calls, no CSE of call results — the original calls each twice):
```
b.x = h*FastSin(angle);  b.y = h*FastCos(angle);
d.x = w*FastCos(angle);  d.y = -w*FastSin(angle);
c.x = x; c.y = y;        (only x,y stored; vec4 16-byte slots at sp+0/+10/+20)
```
Constants: f21 = 16.0 (used in the tail muls), f22 = 1-z2, f20 = 1-z1 (1.0 reused).

16 vector calls (Scale=0x1F9A68 (a0)=a1*f12; Add=0x1F9A10 (a0)=a1+a2;
Sub=0x1F9A28 (a0)=a1-a2). EXACT source statement sequence (delay-slot model:
the delay instruction executes BEFORE the target, so args = post-delay-slot
state). CORRECTED via the expert's delay-slot a0/a1 resolution (the operands
were originally misread):

```
1  Scale(&t,&b,1-z2);  Add(&r0,&c,&t);    # r0 = c + (1-z2)*b
2  Scale(&t,&d,1-z1);  Sub(&r0,&r0,&t);   # r0 = r0 - (1-z1)*d
3  Scale(&t,&b,1-z2);  Add(&r1,&c,&t);    # r1 = c + (1-z2)*b
4  Scale(&t,&d,z1);    Add(&r1,&r1,&t);   # r1 = r1 + z1*d
5  Scale(&t,&b,z2);    Sub(&r2,&c,&t);    # r2 = c - z2*b
6  Scale(&t,&d,1-z1);  Sub(&r2,&r2,&t);   # r2 = r2 - (1-z1)*d
7  Scale(&t,&b,z2);    Sub(&r3,&c,&t);    # r3 = c - z2*b
8  Scale(&t,&d,z1);    Add(&r3,&r3,&t);   # r3 = r3 + z1*d
```
All Scale destinations are `t` (one temp, born stmt1, dies stmt16); the
corners r0..r3 accumulate in place and stay alive to the tail. Int homes:
t=s0, r0=s1, d=s2, c=s3. Stack layout: 8 vec4 slots (128B) + tex 8 (0x80) +
data 4 (0x88) + s0..s8,ra 56 (0x90-0x127) + f20..f27 32 (0x130-0x167) = 0x170.

## Tail (positions)
Per corner K (rK at 0x40/0x50/0x60/0x70), 2 calls to func_001FA6D0 (trunc,
int in v0, truncates f12 in place). The `mul.s f12,f12,16.0` sits in the JAL
DELAY SLOT => it is the CALL ARGUMENT: xK = trunc(rK.x*16.0), yK =
trunc(rK.y*16.0).

```
ix = trunc(rK.x*16.0) + occlViewParams.minX - 8
iy = trunc(rK.y*16.0) + occlViewParams.minY - 8
pos = ((u64)iy<<16) | (u64)ix | ((u64)colour << 32)
```
minX/minY = occlViewParams (0x13E500, named in camera.h) +0x10/+0x14,
RE-LOADED per corner (base s1 kept live: lui s1,0x14; addiu s1,s1,-6912; no
locals). Tail register roles: s0 = x/ix/pos (per corner), s1 = &occlViewParams,
s2 = p (head+16). minX->a0/a1, minY->v1, iy stays in v0. The pos or-chain lands
in s0 (`or s0,s0,v0; or s0,s0,s4; sd s0`). colour is in the HIGH 32 bits
(`dsll32 s4,s4,0` = s4 << 32; AGENTS.md: dsll32 r,r,0 shifts left by 32).

## Packet stores (p = u64 view of the chain after the 4-word header)
```
vu1ChainHead[0] = 0x10000007;  [1]=0;  [2]=0;  [3]=0x50000007;   (sw, 4 volatile loads)
vu1ChainHead = vu1ChainHead + 4;   (RMW: lw; addiu 16; lui at; sw 3840(at))
p[0] = ((u64)0xB400 << 48) | 0x8001;         (sd a1,16(v0))  = 0xB4000000008001
p[1] = ((u64)0xA6A6A6A6 << 11) | 0x106;      (sd a2,8(s2))   = 0x0535353553530106
p[3] = 0x154;                                 (sd a3,24(s2))  stored BEFORE p[2] in binary
p[2] = texId;                                 (ld v0,128(sp); sd v0,16(s2))
p[4] = data;                                  (lw v0,136(sp); sd v0,32(s2)) (lw zero-extends)
p[5] = uMain|vMain;                           (or t0,s6,s5; sd t0,40(s2))
p[7] = uFlip|vMain;  p[6] = pos0;             (UV store BEFORE pos store, all 4 corners)
p[9] = uMain|vFlip;  p[8] = pos1;
p[11] = uFlip|vFlip; p[10] = pos2;
p[13] = 0;          p[12] = pos3;
epilogue: lui v0,0x16; lw v0,3840(v0); addiu v0,112; lui at; sw v0,3840(at)
   => vu1ChainHead = vu1ChainHead + 28 (RMW again; 16+112 = 0x80 total)
```
sd stores write 64 bits; the upper halves are deterministic register junk.

## Key source-form findings (2026-10-01, verified by probe + linked-ELF diff)
- **Constants are explicit shifts in source**: EGC 2.95.2 does NOT
  constant-fold 64-bit shifts of 32-bit constants, so the original wrote them
  as shift expressions. A plain literal `0x0535353553530106` compiles to a
  7-instr decomposition; the shift form gives the original's 5-instr form
  (li;dsll16;ori 0xa6a6;dsll11;ori 0x106). Plain `0xB4008001` literal -> dsll
  (64-bit); the shift form gives `li 0xb400; dsll32 16; ori 0x8001`.
- **texId param is `unsigned long`** (64-bit): the original spills
  `sd a2,128(sp)` (64-bit) in the prologue and reloads `ld v0,128(sp)` for p[2].
- **pos formula**: colour in the HIGH 32 bits:
  `pos = ((u64)iy<<16) | (u64)ix | ((u64)colour << 32)`.
- **Per-corner x-save**: the original keeps x0 in s0 across the 2nd trunc call
  via `move s0,v0` (after the 1st jal, before the 2nd jal), then
  `ix = s0 + minX - 8`.

## Best candidate (ratio 0.8449, 280 words; prologue+trig+vector MATCH)
Signature: 7 float (x,y,width,height,angle,z1,z2) + int uBase, int vBase,
`unsigned long` texId, int colour, int data, `unsigned char` flag0,
`unsigned char` flag1. Locals: `vec4 b,d,c,t,r0,r1,r2,r3; int
uMain,uFlip,vMain,vFlip;`. Pins that close the prologue/trig/vector:
`register float hHome asm("$f25") = height;` and `register float z2Home
asm("$f24") = z2;` (fix the h/z2 trig FPR swap); the tail x-accumulator pinned
`register int px asm("$16");` in a nested block (fixes the per-corner x home to
s0 AND the packet base register to s2); the pos expression written px-first:
`p[K] = ((u64)px | ((u64)py << 16)) | ((u64)colour << 32);` (fixes the pos-or
register to s0). Full source: see working/func_001F5AB0/candidate_084.cpp
(preserved at block time).

## Why it is a wall
The 25 residual blocks are all in the packet/tail/epilogue and are four coupled
EGC 2.95.2 default-scheduler/RA tiebreaks (RTL verified identical, size
correct):
1. Packet constants rotated: orig p[0]->a1, p[1]->a2, 0x154->a3; candidate
   p[0]->a2, p[1]->a3, 0x154->a1 (+1 rotation; the compiler starts from a2).
2. Packet store order: orig `sd p[0]; sd p[1]; <texId ld>; sd 0x154; sd p[2];
   <data ld>; sd p[5]; sd p[4]`; candidate stores 0x154 before the texId load
   and p[5] before p[2].
3. Tail: orig uv->a1 (minX->a0, uv live across the minX load, uv-store AFTER
   `addu v0`); candidate uv->a0 (uv stored before the minX load, freeing a0).
4. Epilogue: orig computes vu1ChainHead+112 into v0 LATE (interleaved with the
   epilogue lq's); candidate computes it into a0 EARLY.

Forcing the original's register assignment with hard pins breaks the natural
allocation: the `asm volatile("" ::: "memory")` clobber form is a parse error in
EGC 2.95.2, and the register pins (packet constants $5/$6/$7, uv $5,
texId/data/finalHead $2) grow the frame 368->384 by spilling the pinned regs,
dropping the ratio from 0.8449 to 0.3993. Same RA wall as the siblings.

## Escalation
- expert (one-shot GPT-6 Astra) invoked: identified the 16 vector-call operand
  misreads (delay-slot a0/a1); corrected -> vector section matches.
- last-resort-decompiler (GPT-5.6 Sol) invoked 2026-10-01 with the full
  280-word dossier: recommended a scoped hard-register/barrier variant (pin
  packet constants $5/$6/$7, uv $5, texId/data/finalHead $2, tied `"+r"`
  barriers). Tested: the `:::memory` clobber form is a parse error; with the
  barriers dropped the pins grow the frame and drop the ratio to 0.3993.
  Confirmed an unmatchable EGC default-scheduler/RA tiebreak wall.

## Action
Source reverted to INCLUDE_ASM; full-ELF parity preserved. Blocker recorded in
decomp_state/blocked.json (id code/game/draw_post_post.cpp:func_001F5AB0).
Re-attempt start point: the best candidate above + the four residual clusters.
