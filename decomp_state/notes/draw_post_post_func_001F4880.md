# func_001F4880 (0x1F4880, 472 B / 118 words) — BLOCKED

Source: `code/game/draw_post_post.cpp:613`, `INCLUDE_ASM(func_001F4880)`.
TU flags: default `-G8 -O2 -ffast-math -fno-exceptions -snas`.

## Semantics (fully decoded, high confidence)
`void func_001F4880(void)` — builds a VU1 effect-quad batch on the stack and hands
the whole 0x90-byte structure to the handwritten `FastDrawQuadReal(void*,int,int)`
(0x1F7D30, drawquad.s). The callee reads a0+0x00..0x30 into vf10-13 (4 x 128-bit
vertices) AND s2+0x40/0x50/0x60/0x70/0x78/0x80/0x88, so nothing in the struct is
dead.

Stack structure (sp-relative):
- sp+0x00: 16 floats = 4 x 128-bit vertices. Inner loop (4 iters) writes 3 floats
  (x,y,z) per vertex at sp+0x00/0x10/0x20/0x30; each vertex's w + the 4th vertex's
  x left unwritten (stale).
- sp+0x40: m0[4] = 0x40808080 (2.0f bit pattern), INTEGER `sw` stores (not float).
- sp+0x50: matrix[8] floats, interleaved from D_0018CAA0 (stride 0x10, reads +8,+0xC).
- sp+0x70: u64 count = 5 (sd store).
- sp+0x78: GetEffectTex(0) low word (tex0); high word stale.
- sp+0x80: u64 = 0xFF90000000000260.
- sp+0x88: u64 = 0x80000044.
- sp+0x90: 4-float normalize buffer; only 2 floats loaded per iter (lq/sq, 64-bit),
  z/w stale (FastVecNormalize still lqc2's all 4).

Algorithm:
1. tex0 = GetEffectTex(0)  (0x1F44B8, returns u64; only $a0 initialized at call -> one-arg).
2. First loop, 4 iters (a2=3, decrement, bgez>=0): m0[i]=2.0f(int);
   matrix[2i]=D_0018CAA0[i*4+2]; matrix[2i+1]=D_0018CAA0[i*4+3].
3. Store count=5, tex0, 0xFF90000000000260, 0x80000044 (BEFORE the init loop).
4. if (effectQuadCount=0x15F474 > 0)  for i in [0,count):
     dir = 2 floats at D_0018E350 + i*0x20; FastVecNormalize(buf,buf,1.0f)
     sc  = *(float*)(D_0018E340 + i*0x20 + 0x0C)
     base= *(u64*)(D_0018E340 + i*0x20)   # 8-byte, same for all 4 inner iters
     for j in [0,4):
       s0=D_0018CAA0[j*4+0]; s1=D_0018CAA0[j*4+1]
       dot = s0*dir.x + s1*dir.y
       v=&vert[j*4]; *(u64*)v = base   # 8-byte
       v[0]+= (s0-dir.x*dot)*sc; v[1]+= (s1-dir.y*dot)*sc
       v[2]-= dir.z*dot*sc   # v[2] stale stack read then swc1
     FastDrawQuadReal(&struct, 0, 0)

## Original register plan (ground truth, generated .s)
- Frame 0x110. Saved: ra + s0..s5 (SIX s-regs).
- s0 = i*0x20 ; s1 = i ; s2 = sp+0x90 ; s3 = &D_0018E350 ; s4 = &D_0018E340 ;
  s5 = &D_0018CAA0. THREE separate base pointers held live across the outer loop.
- BOTH loops run 4 times (counter=3, top-decrement, bgez>=0).
- Init loop: RUNNING source pointer (+=0x10) + 3 running dest pointers + decrement bgez.
- Main loop: running base pointers (s0 += s4), i++ in the normalize-jal delay slot,
  bottom check `slt i<count; bnez`. FP temps f0-f8 only; 2.0f in a3 (int).

## Probe results (tools/decomp_probe.py, default flags)
- 3-iter candidate (my initial): 460 B, 109/118 diffs (EGC 0x100 frame, 5 s-regs).
- 4-iter + one-arg + u64-count + metadata-before-init (last-resort corrections,
  unpinned): 460 B, 113 diffs.
- + s2-s5 register pins (last-resort fallback): 500 B, 114 diffs. Pins fix the frame
  (0x110, s0-s5) but EGC then emits INDEX-based computation (`sll i*16`/`slt/bne`)
  where the original uses RUNNING pointers + a decrement `bgez` counter; the body
  grows 28 B and the init/main-loop indexing stays wrong.
- Flags: -fno-schedule-insns 109d, -fno-schedule-insns2 116d, both 104d/468B. None help.

## Escalation
`last-resort-decompiler` (GPT-5.6 Sol) invoked. It corrected the loop counts (both
loops are 4-iter, not 3), the one-arg GetEffectTex call, the u64 count, and the
metadata-before-init ordering — all applied. It recommended the s2-s5 register pins
as a fallback, which I applied; they fix the frame/saved-register count but not the
indexing/scheduling, so 114 diffs remain. (Its "128-bit lq/sq copies" claim was
verified WRONG against the hex: both copies are 64-bit lq/sq.)

## Why blocked
The residual is EGC 2.95.2 register-allocation + scheduling, identical to the whole
effects family (GetEffectTex__Fii, drawEffectRibbon func_001EDC50, DoGifPaging,
SetupGifPaging__Fi, func_001EE008/328, Camera_UpdateFog func_001EE4B0), all BLOCKED:
the three-separate-base-pointer live range + running-pointer/decrement-counter
loop style + f0-f8 FP temp map is not reproducible from natural C in this build.

Retain INCLUDE_ASM. Full-ELF parity preserved (verified 2026-09-29).
