# DrawTexturedQuad (0x1F5450, 0x184 = 388B, 97 instrs) — BLOCKED

File: `code/game/draw_post_post.cpp`. Blocked 2026-09-30. Retained as
`INCLUDE_ASM`; full boot-ELF parity preserved.

## TARGET NOTE (source-order trap)
The INCLUDE_ASM at line 808 (`func_001F5448`) is a **4-byte dead ghost**
(`sw $2,0($28)` = store v0 to gp+0) right after DrawRectOverlay's
`jr $31; sw $2,-0x5D00($28)` epilogue + alignment nop. It is NOT a function; it is
byte-preservation inline-asm handled together with its parent DrawRectOverlay
(blocked, see notes/draw_post_DrawRectOverlay_FiiiiUl.md). Leave its INCLUDE_ASM;
skip it in target selection. The real target here is DrawTexturedQuad at 0x1F5450
(line 810).

## Semantics
`extern "C" void DrawTexturedQuad(int x0, int y0, int dx, int dy, int u0, int v0,
int du, int dv, unsigned long color, unsigned long tex)` — appends a 0x80-byte
(128B) VU1 textured-quad packet to `vu1ChainHead` (0x160F00) and advances the head
H0→H0+0x80. Plain symbol (symbols.txt:534) ⇒ `extern "C"` (like fadeSetColor).
- Calling convention (verified at call site 0x1EDFA0): 8 int args in {a0-a3, t0-t3};
  the two u64 args on the stack (color=9th → `ld s1,0x20(sp)`, tex=10th →
  `ld s0,0x28(sp)`).
- **Call-free leaf**: 0x20 frame, saves ONLY s1+s0; NO $ra save. C body = zero calls.
- Packet: 4 header words (0x10000007/0/0/0x50000007) at H0+0x00..0x0C; 16-byte
  lq/sq copy of template block D_00160840 → H0+0x10; 12 `sd` at H0+0x20..0x7F
  (tex, 0x154, color, uv0, vert0, uv1, vert1, uv2, vert2, uv3, vert3, 0).
- Head: L1-L4 self-based head loads for the 4 header stores; L5 t5=H0; S1
  head=H0+0x10 (bare `lui at; sw`); lq/sq; S2 head=H0+0x20; 12 sd; L6 reload;
  S3 head=(H0+0x20)+0x60=H0+0x80 (`vu1ChainHead = vu1ChainHead + 24;`).
- occlViewParams (0x13E500): t6=minX(+16), t7=minY(+20), loaded ONCE, reused.
- UVs = `addu` (NOT or), 32-bit: uvN = (uN<<4)+(vN<<20); stored as 64-bit.
- Verts = `or`: vert(x,y)=((x<<4)+minX-8) | (((y<<4)+minY-8)<<16 as 64-bit/dsll16) |
  0xFFFFF000000000 (dli chain). vert0=(x0,y0), vert1=(x0+dx,y0), vert2=(x0,y0+dy),
  vert3=(x0+dx,y0+dy).
- Machine order of the 12 sd: tex (via OLD head t5+0x20), then color BEFORE 0x154,
  then uv0,vert0,uv1,vert1,uv2,vert2,uv3,vert3,0.

## The wall (EGC 2.95.2 RTL-generator / scheduler)
The RTL is fully reproduced and the frame is fixable, but EGC emits the
destructive-arg coordinate/UV shifts (sll 4 / sll 20 / sll 0x14, dsll 16)
immediately after the prologue, whereas the original computes them AFTER the 4
header stores + template copy, interleaving the tag constants (0x10000007/
0x50000007), the head loads (L1-L4), and the occl field loads (minY/minX) in the
prologue. This prologue/body interleaving is not controllable from C source:
moving the destructive updates after the header stores in the source (candidateE)
does not change where EGC emits them (first residual diff stays at 0x1F5464:
candidate `sll` vs original `lui v1,0x1000`).

## Mechanically tested routes (all vs the 388-byte reference .s)
| route | file | bytes | diffs | frame |
|-------|------|-------|-------|-------|
| first form (macro, volatile u64 body, (u32) UV casts) | candidateA | 524 | 131 | 0x70 |
| A -fno-schedule-insns | — | 484 | 120 | 0x20 |
| corrected (cached minX/minY, destructive args, signed 32-bit UV, nonvolatile body stores) | candidateB | 396 | 91 | 0x30 |
| B + color→$17/tex→$16 pins | candidateC | 396 | 89 | 0x30 |
| B + all pins (color/tex/minX→$14/minY→$15) + input-only barrier | candidateD | 396 | 91 | 0x20 |
| D -fno-schedule-insns | — | 396 | 94 | 0x20 |
| D + header-stores-first reorder | candidateE | 396 | 91 | 0x20 |

The corrected form (candidateB/D/E) fixed the two real bugs the first form had:
(1) the DRAWTEX_VERT macro re-read occlViewParams.minX/minY after volatile stores
so EGC conservatively reloaded them (cached into locals + pinned to t6/t7 fixes
this), and (2) the (u32) UV casts introduced dsll32/dsrl32 zero-extend pairs not in
the original (signed 32-bit `long uvN = v0 + u0;` without casts removes them).
The nonvolatile `unsigned long* q` body stores let the scheduler collect the
contiguous `sd` run. With all pins the frame is the correct 0x20 (s0/s1 only,
color in s1, tex in s0), but the 89-91 residual diffs are the prologue/body
interleaving + register-identity cascades.

## Start point for any re-attempt
candidateD.cpp / candidateE.cpp (396B/91, frame 0x20, all pins): the corrected
RTL (cached+pinned minX/minY, destructive arg updates, signed UV, nonvolatile body
stores, double-volatile head idiom, 128-bit `*(volatile CameraQuad*)next =
GIFtag_PrimStSprite;`). Probe with
`python3 tools/decomp_probe.py <cand> code/_generated/nonmatchings/game/draw_post_post/DrawTexturedQuad.s DrawTexturedQuad --define GIFtag_PrimStSprite=0x160840`.
To go further one would need EGC to delay the destructive-arg shifts until after
the header stores + template copy (its RTL generator will not), which is beyond
source-level control.

## Required symbols when eventually integrated
`GIFtag_PrimStSprite = 0x00160840;` in config/symbols.txt (the 16-byte lq/sq
template block; not yet added — the probe uses `--define`).

## Source of the corrected form (candidateD.cpp)
```cpp
#include "common.h"
#include "types.h"
#include "camera.h"

extern volatile u32* volatile vu1ChainHead;
extern CameraQuad GIFtag_PrimStSprite;

extern "C" void DrawTexturedQuad(
    int x0, int y0, int dx, int dy,
    int u0, int v0, int du, int dv,
    unsigned long color, unsigned long tex)
{
    register unsigned long colorreg asm("$17") = color;
    register unsigned long texreg asm("$16") = tex;
    register int minX asm("$14") = occlViewParams.minX;
    register int minY asm("$15") = occlViewParams.minY;
    asm volatile("" : : "r"(minX), "r"(minY));

    dx = ((x0 + dx) << 4) + minX - 8;
    dy = ((y0 + dy) << 4) + minY - 8;
    x0 = (x0 << 4) + minX - 8;
    y0 = (y0 << 4) + minY - 8;

    du = (u0 + du) << 4;
    dv = (v0 + dv) << 20;
    u0 <<= 4;
    v0 <<= 20;

    unsigned long bottom = (unsigned long)dy << 16;
    unsigned long top = (unsigned long)y0 << 16;
    unsigned long mask = 0xFFFFF000000000UL;

    unsigned long vert0 = (unsigned long)x0 | top | mask;
    unsigned long vert1 = (unsigned long)dx | top | mask;
    unsigned long vert2 = (unsigned long)x0 | bottom | mask;
    unsigned long vert3 = (unsigned long)dx | bottom | mask;

    long uv0 = v0 + u0;
    long uv1 = v0 + du;
    long uv2 = dv + u0;
    long uv3 = dv + du;

    vu1ChainHead[0] = 0x10000007;
    vu1ChainHead[1] = 0;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = 0x50000007;

    volatile u32* packet = vu1ChainHead;
    volatile u32* next = packet + 4;
    vu1ChainHead = next;
    *(volatile CameraQuad*)next = GIFtag_PrimStSprite;

    next = packet + 8;
    vu1ChainHead = next;

    unsigned long* q = (unsigned long*)next;

    ((unsigned long*)packet)[4] = texreg;
    q[2] = colorreg;
    q[1] = 0x154;
    q[3] = uv0;
    q[4] = vert0;
    q[5] = uv1;
    q[6] = vert1;
    q[7] = uv2;
    q[8] = vert2;
    q[9] = uv3;
    q[10] = vert3;
    q[11] = 0;

    vu1ChainHead = vu1ChainHead + 24;
}
```
