# func_0023D0E0 / func_0023D110 (code/game/movie/videodec.cpp)

Blocked 2026-09-04: EGC 2.95.2 SN v2.73a cannot reproduce the original schedule
for the "global load + call + return 1" callback shape. Both twins share the
blocker (identical bodies except the callee).

## Semantics (fully understood via Ghidra)
Both are zero-arg movie/mpeg callbacks registered on the mpeg object by
`videoDecCreate` (FUN_0023cac8, vram 0x0023CAC8):
```
FUN_0012bb10(mpeg, 0, 0x23d080, 0);   // mpegError
FUN_0012bb10(mpeg, 1, 0x23d0a8, 0);   // mpegNodata
FUN_0012bb10(mpeg, 2, 0x23d0e0, 0);   // func_0023D0E0  (this note)
FUN_0012bb10(mpeg, 3, 0x23d110, 0);   // func_0023D110  (twin)
FUN_0012bb10(mpeg, 5, 0x23d140, 0);   // func_0023D140
```
- `func_0023D0E0`: `viBufStopDMA((ViBuf*)((u32)*(u32*)0x16120C + 0xD9090)); return 1;`
- `func_0023D110`: `viBufRestartDMA((ViBuf*)((u32)*(u32*)0x16120C + 0xD9090)); return 1;`
`D_0016120C` holds the movie-buffer base address (written by the movie init
FUN_0023a3b8, cleared there too); `+0xD9090` is the resident `ViBuf` inside that
buffer (same offset used by mpegNodata/viBufAddDMA, func_0023D140/viBufGetTs,
videoDecPutTs, mpegError family).

## What matches / what does not
Candidate C (see above) compiles to the EXACT instruction set, registers
(base in $v0, offset 0xD9090 as `lui a0,0xd; ori a0,a0,0x9090`), `addu $a0,$v0,$a0`
in the `jal` delay slot, `sq/lq $ra` at 0(sp) with 0x10 frame, and hoisted
`addiu $v0,$0,1` before `jr $ra`. The ONLY difference is the order of the first
six instructions:

```
original 0x0023D0E0:          EGC v2.73a candidate:
  lui  v0,0x16                  addiu sp,sp,-0x10
  lw   v0,0x120c(v0)            lui   a0,0xd
  lui  a0,0xd                   sq    ra,0(sp)
  addiu sp,sp,-0x10             ori   a0,a0,0x9090
  ori  a0,a0,0x9090             lui   v0,0x16
  sq   ra,0(sp)                 lw    v0,0x120c(v0)
  jal  viBufStopDMA
    addu a0,v0,a0
  lq   ra,0(sp)
  addiu v0,zero,1
  jr   ra
    addiu sp,sp,0x10
```

## Root cause (RTL dumps from `-ddump-rtl-*`)
- Expansion RTL puts the load first in the body: `[load, const, add, call]`.
- After reload the RTL is `[45 sub sp, 47 sq ra, 13 load, 40 const-hi, 41
  const-lo, 19 add, 21 call, ...]` (prologue prepended contiguous at head).
- A post-reload machine-dependent reordering pass (dump "mach", GCSE-like) then
  (a) hoists the invariant `const-hi` (insn 40) up into the prologue region
  (right after `sub sp`, before `sq ra`), and (b) places the load (insn 13) at
  its LAST use (after `const-lo`). Resulting RTL:
  `[45, 40, 47, 41, 13, 19, 21, ...]`.
- Because the load is now AFTER the `sq ra` store in RTL order, the final
  scheduler records a conservative memory dependence `13 -> 47`
  (load `[0x16120c]` vs store `[sp+0]`, aliasing not provable). The final
  scheduler is priority-driven and emits in ready order; the load is gated
  behind the store, so it always lands last.
- The original binary has the load FIRST and the prologue split around the
  constant pair (`C1, P1, C2, P2`). That order is only reachable if the load is
  before the `sq ra` in the post-reload RTL (no `13 -> 47` dependence) AND has
  top priority at t=1. EGC v2.73a never produces a post-reload RTL with the
  body load ahead of the prologue store: the prologue is inserted contiguous at
  the function head, and the reordering pass moves the load to its last use.

## Attempts (all give the EGC-v2.73a order above)
Direct expr `*(u32*)0x16120C + 0xD9090`; reversed operand order; local base
var; local addr var; two locals (base+off); `base += off` two-statement;
ViBuf* local; `char*` cast; volatile load; `ViBuf*`/`char*` pointer arithmetic
(wrong add width: daddu/scaled); C (not C++) file; unused callback params
`(void*,void*)`; flag variants -O1/-O3/-fno-gcse/-fno-schedule-insns/-G0/-G16.
(-O1 changes the order to `[sub sp, sq, load, const, call]` — load still not
first — and -O1 is not a project flag.)

## Precedent check
No matched function in the repo has the "frame + global load + call" shape
(snd_CloseMovieSound matches for "frame + call + const args" but has no load;
ErrMessage/func_001F6250 match for "frame + call + const ADDRESS arg" but have
no load). This is the first function requiring a load scheduled ahead of the
prologue `sq`, so the EGC-version scheduler/dependence difference is untested
elsewhere.

## Also likely blocked (same shape, not individually verified)
- `mpegNodata__FP7sceMpegP13sceMpegCbDataPv` (0x0023D0A8): switchThread(); then
  load base; viBufAddDMA(base+0xD9090); return 1.
- `func_0023D140` (0x0023D140): 0x40 frame, viBufGetTs(base+0xD9090, &ts); return 1.

Status: BLOCKED on toolchain. Keep INCLUDE_ASM until the EGC scheduler
difference is resolved (newer/older EGC build, or a source construct that
changes the post-reload RTL order of load vs prologue store).
