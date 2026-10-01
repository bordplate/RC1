# DrawSprite (func_001F55D8, 0x230 = 560B, 140 instrs) — BLOCKED

File: `code/game/draw_post_post.cpp` (INCLUDE_ASM at line 833). Blocked
2026-10-01. Retained as `INCLUDE_ASM`; full boot-ELF parity preserved.
Queue id: `code/game/draw_post_post.cpp:557:func_001F55D8`.

## Semantics (verified against ground-truth objdump of assets/boot_elf.elf
plus the Deadlocked descendant `DrawSprite__FffffiiiiUlUl`,
reference/dl/FUNCTIONS.txt ~line 121310)

`void DrawSprite(float scX, float scY, float scW, float scH, int txU, int txV,
int txW, int txH, u64 rgba, u64 tex)` — floats in f12-f15, the four ints in
a0-a3, rgba in t0, tex in t1 (all five int args register-passed; 0xE0 frame,
s0-s8 + ra + 5 FPU words saved, rgba spilled `sd t0,0(sp)`).

- x0=trunc(scX*16)+minX-8, x1=trunc((scX+scW)*16)+minX-8, y0=trunc(scY*16)+minY-8,
  y1=trunc((scY+scH)*16)+minY-8; trunc=`func_001FA6D0(float)` (4 calls);
  minX/minY = occlViewParams (0x13E500) +0x10/+0x14.
- Appends a 0x80-byte VU1 packet to `vu1ChainHead` (0x160F00):
  header words 0x10000007/0/0/0x50000007 at +0x00..+0x0C; 128-bit `lq/sq` copy of
  the 16-byte template 0x160840 (a GIFtag prim-sprite record; lives in the `lit`
  Splat section, inside the gp window) to +0x10; S1 head=slot+4, S2 head=slot+8
  (GPREL `lui at; sw r,3840(at)`); u64 payload: +0x20 tex, +0x28 0x154, +0x30 rgba,
  +0x38 uv0, +0x40 vert0, +0x48 uv1, +0x50 vert1, +0x58 uv2, +0x60 vert2, +0x68 uv3,
  +0x70 vert3, +0x78 0; S3 final head = head+0x60 in the epilogue.
- UVs are 32-bit (`addu`/`sll`, NO zero-extends — the sd stores the upper halves
  as original garbage): uv0=txV<<20+txU<<4, uv1=txV<<20+(txU+txW)<<4,
  uv2=(txV+txH)<<20+txU<<4, uv3=(txV+txH)<<20+(txU+txW)<<4 (two terms, NOT uv1+uv2).
- Verts: `(u64)x | ((u64)y << 16) | 0x00FFFFF000000000ULL` with 64-bit `dsll`
  shifts; EGC materializes the mask as `li 0xffff; dsll 16; ori 0xf000; dsll 24`.
  **The mask is 0x00FFFFF000000000 (14 hex digits), matching the Deadlocked literal
  `0xfffff000000000`; the 0x000FFFFF00000000 form (dsll 20) is a different value and
  was the first-form bug (see candidateL).**

## Original register homes (ground truth)
prologue: `sq s8` early @0x5F0, moves s7=a0 @0x600, s6=a1 @0x608, s5=a3 @0x610,
s4=a2 @0x618, **move s8,t1 in the delay slot of jal #1**; base occlViewParams ->
**s0** @0x654 (after call #1); x0->s3, x1->s2, y0->**s1**, y1->**a1** (minY-for-y1
reloaded into **a3** @0x6A8); tag1->v1, tag2->**a2**; head loads L1->a0, L2->**v0**,
L3->v1, L4->**v0**, L5->**a3** (a3 = head, also the tex-sd base); template addr in
**a0** with the dependent lui/addiu pair split 16 instructions apart (hi @0x6CC,
lo @0x6EC); `lq v0,0(a0); sq v0,0(v1)` where v1 = addiu(a3,16) = materialized p4
(used by S1 too); data ptr = addiu(a3,32) reusing v1; 0x154 -> a0 (`li` after the lq
freed a0); mask -> **v0**; uv3 -> **t0**; uv1/uv2/uv0 destructive on s4/s5/s6; rgba
reload `ld v0,0(sp)` reusing v0 after the mask dies; epilogue **lq ra FIRST**, then
lui v0; lw v0; lq s8; addiu v0,96; lq s7...s0; lwc1 f24..f20; lui at; sw v0; jr.

## The wall (EGC 2.95.2 RTL/scheduler, same class as the three siblings)
The RTL is fully reproducible (frame, op sequence, destructive reuses, value
semantics all match) but EGC 2.95.2's final register allocation + list
scheduling diverges in five independent, mechanically stable conflicts:
1. Prologue ready-move rotation: EGC emits the arg moves s8=tex FIRST and
   s4=txW LAST (in the jal delay slot); the original is s7,s6,s5,s4 then
   s8 in the delay slot. Final homes are otherwise correct.
2. Base/y0/y1/tag2 home cluster: EGC wants base->s1, y0->s0, y1->v0 (destructive
   on the call result), minY-for-y1->a1, tag2->a0; the original is s0/s1/a1/a3/a2.
   (candidateZ3's scoped `$a3`/`$a2` pins fix the a3-reload and tag2 homes but not
   the rest; s-register pins are silently ignored in this 10-arg context.)
3. Template address + destination fold: EGC keeps the template lui/addiu adjacent
   (both as an unsplittable `la` pseudo for the 8-byte declaration and as a
   split-but-adjacent pair for the 16-byte one) and folds the 128-bit store to
   `sq r,16(head)`; the original splits the dependent pair 16 instructions apart
   and uses the materialized p4 as the sq base register.
4. Payload temporary cycle: EGC cycles mask->v1, 0x154/rgba->a0, uv3->a2,
   vert-pres->a3/v0; the original cycles mask/rgba->v0, uv3->t0, vert-pres->a2/a1.
5. Epilogue order: even with the full memory barrier that stops the final
   volatile head read (L6) from hoisting into the sd block, EGC orders
   `lui v0; lw v0; lq ra; addiu v0,96; lq s8`; the original is
   `lq ra; lui v0; lw v0; lq s8; addiu v0,96`. The compiler-generated restores
   are not source-controllable.

## Mechanically tested routes (tools/decomp_probe.py, default flags unless noted)
| candidate | shape | diffs |
|-----------|-------|-------|
| A/B | first full-body forms (shifts hoisted into prologue) | 102 |
| C | header stores before shifts + input-only barrier after them (defeats the hoist) + wrong-width u64 uv3 | 82 |
| D | C + y1 pinned `$5` + sd reorder (pin ignored, mapping perturbed) | 85 |
| E | C + p4 variable + `"+r"` barrier + (u32*) cast | 81 |
| F | C + int 32-bit uvs with correct two-term uv3 + sd reorder (correct semantics) | 81 |
| G | F + destructive param uv reassignment | 86 |
| H/I/J | F + p4 var (no barrier) / y1 as 3 statements / mask pinned `$2` | 81 (byte-identical to F) |
| K | F + occl base pinned `$16` via register struct* | 97 |
| L | F + corrected mask constant 0x00FFFFF000000000 | 81 |
| M | L + p4 CameraQuad* + `"+r"` barrier (expert exp 1) | 90 |
| N | L + full memory barrier before final advance (expert exp 3a; L6 fixed) | 80 |
| P | L + tex pinned `register u64 asm("$30")` (expert exp 4) | 127 |
| R | L + `gifTagPrimSprite[4]` declaration (split lui/addiu RTL) | 81 |
| S | N + R | 80 |
| **Z3** | **verts after 2nd head update + split 4th conversion + scoped `y1Min` pinned `$a3` + tag2 pinned `$a2` + barrier after first 3 UV intermediates (last-resort)** | **65** |
| F -fno-schedule-insns / -fno-schedule-insns2 | | 112 / 117 |
| Z3 -fno-gcse | | 65 |
| Z3 -mno-split-addresses / -fno-expensive-optimizations | | 82 / 89 |
| Z3 -fno-schedule-insns2 / -fno-schedule-insns | | 106 / 107 |

Also tested without improvement (Z3 base): -fno-rerun-cse-after-loop,
-fno-cse-follow-jumps, -fno-cse-skip-blocks, -fno-strength-reduce;
-fno-delayed-branch grows the function; alternative -mcpu rejected by the EE
compiler. Pin isolation probes confirm EGC honors `register int asm("$N")` pins
only in low-register-pressure functions; in this 10-arg 0xE0-frame context they
are silently ignored (s-registers) or perturb allocation (`$2`/`$7`-scoped pins
partially work: Z3's `$a3`/`$a2` pins landed, full-lifetime ones did not).

## Verified mechanical facts from this investigation
- The float prologue (f20-f24 moves, `mtc1` 16.0 constant, mul.s/add.s) matches
  byte-for-byte in every candidate from B on.
- EGC elides SI->DI zero-extends at `sd` (matches the original's garbage upper
  halves); the source must use 32-bit int intermediates, not u64.
- EGC preserves source statement order for the payload `sd` block; the machine
  sd order (tex, 0x154, vert0, uv0, uv1, vert1, uv2, rgba, vert2, uv3, vert3, 0)
  is reproducible by statement order.
- `la` pseudo (unsplittable) is emitted for <=8-byte externs under -G8; a
  16-byte declaration gives split lui/addiu RTL but the scheduler keeps the
  dependent pair adjacent.
- The `asm volatile("" : : "r"(txU), "r"(txV), "r"(txW), "r"(txH));` input-only
  barrier after the header stores is what defeats the UV-shift hoist into the
  prologue (102 -> 82 class of results).
- A full `asm volatile("" : : : "memory");` barrier before the final head advance
  keeps L6 out of the sd block (81 -> 80).
- Splat `.s` third-comment hex fields are byte-reversed (LE word reversed); the
  probe candidate.json columns are in that format. Ground truth is always
  objdump of assets/boot_elf.elf.

## Last-resort escalation
`last-resort-decompiler` (GPT-5.6 Sol) invoked 2026-10-01 with the full dossier
(original disasm, candidateN source, 80-diff clusters, all 17 prior candidates +
flag table, EGC empirical facts, sibling blocked notes). It reproduced the probe
harness, found candidateZ3 (65 diffs, the new local minimum, artifacts in
working/draw_post_DrawSprite/probeZ3/), mechanically tested the additional flag
matrix and pin/barrier variants listed above, and returned a definitive wall
verdict: three simultaneous, mutually regressing codegen conflicts (prologue
move rotation; inseparable template-address pair + destination folding;
compiler-generated epilogue restore ordering) with no source control that fixes
one without regressing another. An inline-assembly body could reproduce the
bytes but is not a matching decompilation and offers no advantage over the
existing generated assembly. Verdict accepted: retain INCLUDE_ASM.

## Resume notes (if a future compiler/flag discovery reopens this)
Start from candidateZ3.cpp (65 diffs). The residual clusters and their required
original homes are listed above; the prologue rotation (s8/tex first) and the
template block (adjacent lui/addiu + folded sq + lq->a2) are the largest single
winners if either becomes source-controllable. Siblings with the same wall:
DrawRectOverlay (0x1F52A0), DrawTexturedQuad (0x1F5450), DoGifPaging (0x1F4398).
