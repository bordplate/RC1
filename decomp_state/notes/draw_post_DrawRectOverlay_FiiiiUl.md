# DrawRectOverlay_FiiiiUl (0x1F52A0, 0x1A8 = 106 words) — BLOCKED

File: `code/game/draw_post_post.cpp`. Blocked 2026-09-30. Retained as
`INCLUDE_ASM`; full boot-ELF parity preserved.

## Semantics
`void DrawRectOverlay(int top, int bot, int left, int right, unsigned long color)`
(color is a 64-bit value in one GPR, a4/$8) appends a 24-word (0x60) VU1 packet to
`vu1ChainHead` (0x160F00), advancing the head +0x60 total (four +0x10 advances; the
final store is a GPREL `sw $2,-0x5D00($28)` in the `jr $31` delay slot via the plain
`vu1ChainHeadStore` alias). No stack frame. Packet layout:
- w0-3: `0x10000005, 0, 0, 0x50000005`
- w4-7: 128-bit lq/sq copy of `GIFtag_PrimRgba`@0x160820
  (`00008001 24000000 00000010 00000000`); then `sh` low16 of w4 = 0x8001 (no-op patch).
- w8-11: `0x144, 0, color, 0` (two 64-bit sd).
- w12-15: 128-bit copy of `GIFtag_Vert`@0x160830
  (`00008002 14000000 00000004 00000000`); then `sh` low16 of w12 = 0x8004 (0x8002->0x8004).
- w16-23: four 64-bit coord pairs (top,left),(top,right),(bot,left),(bot,right):
  `pair = (long)(ycoord*16 + occlViewParams.minX - 8)
         | (long)(xcoord*16 + occlViewParams.minY - 8) << 16
         | 0xfffff000000000`.
  minX = +0x10 (0x13E510), minY = +0x14 (0x13E514). Verified byte-exact by Ghidra and
  the Deadlocked decomp (`reference/dl/FUNCTIONS.txt` DrawRectOverlay__FiiiiUl,
  `tables.cpp:744-752`: `Scrn.ofs_x/ofs_y`). Lombyte has NOT decompiled it
  (`reference/Lombyte` fun_001f52a0.c is an assembly placeholder).

## Verified instruction forms (so a future attempt does not re-derive them)
- s16 patch form: the original materializes 0x8001 / 0x8004 as SIGNED
  `addiu $9,$0,-0x7FFF` / `addiu $8,$0,-0x7FFC`. Confirmed in isolation
  (working/draw_post_DrawRectOverlay/s16test.cpp): `*(volatile s16*)p = 0x8001`
  compiles to exactly `addiu r,$0,-0x7FFF; sh`. A plain `*(u16*)p` gives `ori` (wrong).
  So the patches are stored through a **signed 16-bit pointer**.
- mask build: `0xfffff000000000` is emitted as a `dli` pseudo that ps2eeas expands to
  the original's exact 4-instruction chain
  `ori r,$0,0xFFFF; dsll 16; ori r,0xF000; dsll 24`. A folded mask constant is fine.
- All head reads are self-based `lui r,%hi(vu1ChainHead); lw r,%lo(r)`; intermediate
  head writes are split `lui $1,%hi; sw $3,%lo($1)`; only the final write is GPREL.

## The wall (EGC 2.95.2 RA / pre-RA-scheduler)
The RTL is fully understood and reproduced, but EGC's global register allocator and
pre-RA scheduler place hoistable constants in different registers than the original,
cascading into ~50-70 word diffs. The original keeps each hoistable at a tight
point-of-use with a specific narrow register home; EGC 2.95.2 (default flags,
`-G8 -O2 -ffast-math -fno-exceptions`, ps2eeas) hoists
`{four sll arg,4; mask; 0x8001; 0x144; 0x8004; la(GIFtag_Vert); la(occlViewParams)}`
and assigns them registers that do not line up with the original's homes
(0x50000005 $9<->$10, blockA $10<->$9, 0x8001 $9<->$2, 0x144 $12<->$9, 0x8004 $8<->$3,
occl base $9<->$8, mask $11<->$9, pair head base $8/$10<->$13/$14).

## Mechanically tested routes (all vs the 424-byte reference .s)
| route | file | bytes | diffs | first |
|-------|------|-------|-------|-------|
| structural best (DL-form pair expr, s16 patch, 128-bit copy) | candidateD.cpp | 424 | 70 | 0x1f52b0 |
| qword* volatile-ptr / non-vol pointee (last-resort r1) | candidateF.cpp | 528 | 125 | 0x1f52bc |
| qword* both volatile (last-resort r2) | candidateG.cpp | 536 | 132 | 0x1f52a8 |
| instruction-producing asm, natural alloc (last-resort r3) | candidateH.cpp | 424 | 67 | 0x1f52b0 |
| instruction-producing asm, fixed reg homes (last-resort r4) | candidateI.cpp | 424 | 52 | 0x1f52b0 |

Also tried and worse: `-fno-schedule-insns` (90-94), `-fno-schedule-insns2` (80-87),
both (92), and zero-byte `asm volatile("" : : "r"(x))` scheduling barriers
(candidateE.cpp, 428B/99 — the barriers ADD a word). The qword routes fail outright:
EGC spills the qword pointer(s) to a 16-byte stack frame (528/536 B), because the
packet is built in-place at the running head, not from one fixed qword base.

The route-4 residual 52 words are pure RA: header 0x50000005/blockA swap, section-1
head load ($13 vs $11) + a small reorder, and the pair-section base registers
(p12 pair1 `sd $2,16($13)` / pairs2-4 `sd $*,off($14)` vs original pair1
`sd $2,0x10($8)` / pairs2-4 `sd $*,off($10)`). The structure matches (pair1 from
old-head+0x10, pairs2-4 from new-head+offsets); only the base registers differ. The
original reuses $8 (color's argument register, dead after the section-2 `sd`) for the
section-4 old-head load; EGC instead uses $13 and cannot be forced to reuse $8 without
conflicting with the live `color` argument in earlier sections. No flag, pin, or
barrier tested closes this.

## Start point for any re-attempt
candidateI.cpp (route 4, 424B/52) is the closest: instruction-producing asm for the
seven hoistables at their original points + `register ... asm("$N")` homes
(patch1=$9, w8=$12, blockB=$10, patch2=$8, params=$9, mask=$11) + DL-form `(long)`
pair expressions + `*(volatile s16*)` signed patches + `*(const CameraQuad*)` 128-bit
copy + `next = packet+4; vu1ChainHead = next;` head idiom + GPREL final store via
`vu1ChainHeadStore`. To go further one would need EGC to reuse $8 for the section-4
old-head load and $10 for the new-head base (a specific inter-section register-reuse
decision the compiler does not make), which is beyond source-level control.

## Required symbols when eventually integrated
`GIFtag_PrimRgba = 0x00160820;` and `GIFtag_Vert = 0x00160830;` in config/symbols.txt
(neither exists yet; the probe uses `--define`). `vu1ChainHeadStore` = 0x160F00 is the
existing GPREL alias in config/linker_aliases.ld.

## Re-attempt start-point source (candidateI.cpp, route 4 = 424B/52)
Probe with: `python3 tools/decomp_probe.py <this file> \
code/_generated/nonmatchings/game/draw_post_post/DrawRectOverlay_FiiiiUl.s \
DrawRectOverlay_FiiiiUl --define vu1ChainHeadStore=0x160F00 \
--define GIFtag_PrimRgba=0x160820 --define GIFtag_Vert=0x160830`.

```cpp
#include "common.h"
#include "types.h"
#include "camera.h"

extern volatile u32* volatile vu1ChainHead;
extern volatile u32* vu1ChainHeadStore;
extern u64 GIFtag_PrimRgba[];
extern u64 GIFtag_Vert[];

void DrawRectOverlay(int top, int bot, int left, int right, unsigned long color)
    asm("DrawRectOverlay_FiiiiUl");

void DrawRectOverlay(int top, int bot, int left, int right, unsigned long color)
{
    vu1ChainHead[0] = 0x10000005;
    vu1ChainHead[1] = 0;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = 0x50000005;
    volatile u32* packet = vu1ChainHead;
    volatile u32* next = packet + 4;
    vu1ChainHead = next;
    *(volatile CameraQuad*)next = *(const CameraQuad*)GIFtag_PrimRgba;
    register int patch1 asm("$9");
    register u64 w8 asm("$12");
    asm volatile("addiu %0,$0,-0x7fff\n\taddiu %1,$0,0x144" : "=r"(patch1), "=r"(w8));
    *(volatile s16*)next = patch1;
    packet = vu1ChainHead;
    next = packet + 4;
    vu1ChainHead = next;
    register const u64* blockB asm("$10");
    asm volatile("lui %0,%%hi(GIFtag_Vert)\n\taddiu %0,%0,%%lo(GIFtag_Vert)" : "=r"(blockB));
    *(volatile u64*)next = w8;
    *(volatile u64*)(next + 2) = color;
    packet = vu1ChainHead;
    next = packet + 4;
    vu1ChainHead = next;
    *(volatile CameraQuad*)next = *(const CameraQuad*)blockB;
    register int patch2 asm("$8");
    register struct OcclViewParams* params asm("$9");
    asm volatile("addiu %0,$0,-0x7ffc\n\tlui %1,%%hi(occlViewParams)" : "=r"(patch2), "=r"(params));
    *(volatile s16*)next = patch2;
    asm volatile("addiu %0,%0,%%lo(occlViewParams)" : "+r"(params));
    packet = vu1ChainHead;
    next = packet + 4;
    vu1ChainHead = next;
    volatile u64* vert = (volatile u64*)next;
    register u64 mask asm("$11");
    asm volatile("sll %0,%0,4\n\tsll %1,%1,4" : "+r"(top), "+r"(left));
    asm volatile("dli %0,0xfffff000000000" : "=r"(mask));
    asm volatile("sll %0,%0,4" : "+r"(right));
    vert[0] = (long)(left + params->minX - 8) | (long)(top + params->minY - 8) << 16 | mask;
    asm volatile("sll %0,%0,4" : "+r"(bot));
    vert[1] = (long)(right + params->minX - 8) | (long)(top + params->minY - 8) << 16 | mask;
    vert[2] = (long)(left + params->minX - 8) | (long)(bot + params->minY - 8) << 16 | mask;
    vert[3] = (long)(right + params->minX - 8) | (long)(bot + params->minY - 8) << 16 | mask;
    vu1ChainHeadStore = vu1ChainHead + 8;
}
```
