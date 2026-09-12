# ExecuteCamPostUpdFuncs (0x001EBCF0, 0x6C bytes) — research note (NOT matched)

Not matched as of 2026-09-10. Recorded so the investigation is not lost. This is a
research note, not a blocker entry (no last-resort-decompiler invocation yet).

## Semantics

Iterates the camera post-update routine table and calls each registered routine,
then clears the count.

```
CamPostUpdRoutineCnt   = 0x15EF8C  (in-window .lit; accessed via absolute lui/lw)
CamPostUpdRoutines     = 0x1892B0  (out-of-window .data; array of fn pointers)
```

Live body:
```c
int i = 0;
if (CamPostUpdRoutineCnt > 0) {
    cursor = CamPostUpdRoutines;   // s1 in the original
    fn = cursor[0];                // v1 in the original
    do {
        i++;                       // s0 in the original
        fn();                      // jalr v1
        cursor++;                  // in the jalr delay slot
        // reload CamPostUpdRoutineCnt
        // if (i < cnt) loop;  fn = cursor[0] in the branch delay slot
    } while (i < CamPostUpdRoutineCnt);
}
CamPostUpdRoutineCnt = 0;
```

- One caller: 0x1EC504 (in FUN_001ec420, the camera update), `jal 0x1ebcf0`.
  The caller leaves a0 = s0 (the camera state pointer) stale-but-live across the
  call, so the original takes a pointer param in a0 (see dead tail below).
- Symbol is unmangled (C linkage) — declare `extern "C"`.

## Dead tail (func_001EBD60, 0x18 bytes, orphan INCLUDE_ASM)

Unreachable, right after the epilogue (`jr ra; addiu sp` at 0x1EBD54/58):
```
1ebd60: move v0, zero
1ebd64: nop
1ebd68: move v0, zero
1ebd6c: nop
1ebd70: sw  zero, 0(a0)
1ebd74: nop
```
No jal/j to 0x1EBD60 anywhere (raw-encoding scan); Ghidra has no function there.
The `sw zero,0(a0)` + `move v0,zero` pair indicates the original C had a pointer
param (a0) and a value return (v0) that EGC dead-code-eliminated into the tail.
Same "dead tail of the preceding function" artifact family as the ~46 phantoms
(see help_msg_string__Fi.md).

## Blocker: EGC codegen does not reproduce the original

Compiled candidates (project flags `-G8 -O2 -ffast-math -fno-exceptions`) with the
loop written as while(true)+break, do-while, Ghidra order, param/return variants,
and declaration reordering. The epilogue always matches, but the prologue/loop never
does. Persistent differences (original vs. my EGC):

- **s-register allocation is flipped and invariant to source order**: original keeps
  `i` in **s0** and the cursor in **s1**; my EGC keeps `i` in **s1** and the cursor
  in **s0** for every variant tried (i declared first/last, r declared first/last).
  fn is v1 in both. This single difference cascades into the slt operand order
  (`slt v0,s0,v0` vs `slt v0,s1,v0`), the save/restore order, and the delay-slot
  choices.
- **Loop scheduling**: original has `i++` standalone at loop top and `cursor++` in
  the jalr delay slot, with a **bnezl** (branch-and-link) loop-back. My EGC puts
  `i++` in the jalr delay slot and `cursor++` in the main flow, with a plain **bnez**.
  The while(true)+break form additionally gets **peeled/rotated** by EGC (first fn
  load hoisted to a direct reloc load + an unconditional `b` to skip the in-loop
  load); the do-while form avoids the peel but still has the register/scheduling
  differences.
- **cnt load hoisting**: original hoists the `CamPostUpdRoutineCnt` load before the
  `sq` saves; my EGC emits it after.
- **Dead tail**: with `int f(int *x)` + `*x = 0; return 0;`, EGC saves x in s2
  (frame grows 0x30→0x40) and keeps both stores LIVE (no dead tail). It does not
  reproduce the original's 3-instruction dead tail (two `move v0,zero` + one
  `sw zero,0(a0)`) with the 0x30 frame.

## Ideas for a future attempt

- The bnezl + i-in-s0 + cursor-in-s1 combination may require a specific C structure
  not yet tried (e.g. the counter and cursor introduced in a different scope, or the
  loop written so the counter is the "outer" register).
- The dead tail's two `move v0,zero` may come from two zero-initialized locals or a
  `return (x[0] = 0)`-style expression, not a plain `return 0`.
- Try `-fno-schedule-insns` / `-fno-schedule-insns2` (per the menu-family flag scope
  note) in case the scheduling differences are scheduler-driven — but the register
  allocation difference would still remain.
- A `decomp-researcher` pass on the exact Ghidra RTL / the original Insomniac source
  idiom for this loop would be the next step.

## Verification status

- No candidate matched; the `INCLUDE_ASM` fallback for ExecuteCamPostUpdFuncs and
  its orphan func_001EBD60 are both retained. No source changes were made for this
  function. Full boot parity is unaffected.
