# camera_Cam_InterpValues__FffPffff (0x1EBD78, 0xE4)

Matched 2026-09-22.

## Signature

`float Cam_InterpValues(float current, float target, float* offset, float step, float decay, float limit)`
(cfront `__FffPffff`; Ghidra's parameter order is wrong — it puts the pointer
last. True layout: f12=current, f13=target, a0=offset, f14=step, f15=decay,
f16=limit.)

## Behavior

Exponential approach: `v = *offset + step*(target-current) - decay*(*offset)`;
`*offset = v` (unconditionally); if `limit != 0.0f`, clamp v to the +/-limit
band; then clamp *offset to the gap magnitude `a = FastAbsF(target-current)`:
`*offset > a` stores `a`, the lower clamp is *supposed* to store `-a` when
`*offset < -a` but the original compiler clobbers `-a` (in f0) with a
delay-slot `lwc1 f0,0(a0)` reload, so the store writes `-*offset`. Returns
`current + *offset`.

All four call sites (in func_001ED470) pass limit = 0.0f, so the limit band
is dead in the boot image.

## Callee rename

`func_001F99C0` (2-instruction `jr ra; abs.s f0, f12`) in the handwritten
game/fastfunc region renamed to `FastAbsF = 0x1f99c0;` in config/symbols.txt;
prototype in code/include/common.h. Splat regenerated fastfunc.s and every
referencing nonmatching .s with the new label. No raw data references to
0x1F99C0 exist.

## Why the C shape

First clamp block, byte structure of the original:

```
add.s  f1, f1, f14        ; v = old + f14 (hoisted ahead of the guard)
bc1t   -> exit             ; C0 = (limit == 0.0) from a hoisted c.eq.s
  swc1 f1, 0(a0)           ; *offset = v  (delay slot: always executes)
c.lt.s f16, f1             ; (limit, v)
nop
bc1tl  -> exit             ; guard 2: branches PAST guard 3
  swc1 f16, 0(a0)
neg.s  f16, f16
c.lt.s f1, f16             ; (v, -limit)
nop
bc1tl  -> exit             ; guard 3
  swc1 f16, 0(a0)
```

Two EGC scheduling facts decided the C form:

1. `*offset = v;` must be OUTSIDE the `if (limit != 0.0f)` block. Inside it
   (probe2), EGC treats the store as conditional and restructures the block
   (moves the store after the first cmp, merges the two stores under one
   branch — 6 word diffs). Outside, the unconditional store is legally pulled
   into the guard branch's delay slot and `add.s` hoists ahead of it.
2. The two clamp stores must be an if/else-if chain, not two independent
   ifs. Independent ifs (probe3) make guard 2 branch to the instruction after
   its own store (the `neg.s`), so execution falls into guard 3; the original
   guard 2 branches to the common join past guard 3, which is exactly what
   `if (v > limit) *offset = limit; else if (v < -limit) *offset = -limit;`
   produces (both guards `bc1tl` to the same label, stores in the delay
   slots). `if (limit < v)` compiles to the same c.lt.s word; `if (v > limit)
   ; else *offset = limit;` flips the branch to bc1f.

Probe ladder: probe1 (both stores inside, if/else-if: 11 diffs) -> probe2
(stores inside, independent ifs: 6 diffs) -> probe3 (v-store outside,
independent ifs: 1 diff = guard 2 branch target) -> probe7 (v-store outside,
if/else-if: match).

## Gotchas learned

- `f0` is the FP return register ($v0 f0). The first `FastAbsF` result lives
  in f0 and is legitimately reused by the later `c.lt.s f0, f1` and the
  then-branch `swc1 f0` even though intervening calls reissue the argument
  move in their delay slots. Do not read the delay-slot `mov.s f12, f20` as
  clobbering a live value: it sets the NEXT call's argument before that call
  starts.
- The else branch's real miscompilation: EGC schedules `lwc1 f0, 0(a0)` into
  the `bc1fl` delay slot, destroying the `-a` left in f0 by `neg.s`; the
  later `neg.s f0, f0; swc1 f0` then stores `-*offset` instead of `-a`.
  The natural C (`if (*offset < -a) *offset = -FastAbsF(delta);`) reproduces
  the identical scheduling; see probe_z.cpp (m1/m2) for the minimal form of
  the call-result-through-f0 behavior.
- `tools/decomp_probe.py` prints difference words in Splat byte-reversed
  style: the real little-endian word is the byte-reversal of the printed
  hex (e.g. printed `06000345` is word 0x45030006 = bc1tl). Decode branch
  words from the .o/ELF, not from the printed diff, or the l-bit reads
  backwards.

## Verification

probe7 matched all 0xE4 bytes standalone (default flags: -G8 -O2
-ffast-math -fno-exceptions -snas, camera.o has no private flags). Full
build + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
