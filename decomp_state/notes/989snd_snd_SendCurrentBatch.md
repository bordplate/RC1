# snd_SendCurrentBatch (code/989snd/ee/989snd.c) — BLOCKED 2026-09-06

`void snd_SendCurrentBatch(void)` at vram `0x12E9D8`, 0x114-byte body (69 words) plus a
0xC-byte dead-tail artifact `snd_UnkFunction_0012eaf0` at `0x12EAF0`.

## Identity / logic

Double-buffered sound batch dispatcher. `D_0015ECC0` (gp-0x7F40) is the 0/1 buffer
index. `D_0015ECA0[2]` (P, ptrs), `D_0015ECC8[2]` (Q, ptrs), `D_0015ECB8[2]` (R, ints)
are the two-buffer state. `D_0015EBC0` (0x15EBC0) and `D_00153D20` (0x153D20) are OUT of
the gp window (gp=0x166C00) and are passed as ADDRESS CONSTANTS (lui/addiu), not loaded
values.

```c
snd_PrepareReturnBuffer(Q[idx], *P[idx]);          // idx = D_0015ECC0
while (func_0011B3B8((int)&D_0015EBC0)) {          // ptr arg, out-of-gp
    func_00116078((int)&D_00153D20);               // ptr arg, out-of-gp
    FlushCache(0);
}
func_0011B1C8((int)&D_0015EBC0, 0x4D, 1, P[idx], 0x1000 - R[idx],
              Q[idx], (int)*P[idx] * 4 + 8, 0, 0);  // 9 args
f = D_0015ECC0 != 1;
D_0015ECC0 = f;
*P[f] = 0;
R[f] = 0xFFC;
D_0015ECC4 = 1;   // <- emitted as DEAD by the original EGC (see tail)
```

`func_0011B3B8`/`func_0011B1C8` are defined in `code/_generated/sce/lib.s` (uppercase B —
a lowercase `extern` fails to link). `func_00116078` is in glibc.s.

## What matches (verified word-by-word, reloc fields as wildcards)

Words 0-49 (prologue, first call, loop, 9-arg setup) are BYTE-IDENTICAL to the original
modulo relocations. The C form above reproduces them exactly, including:
- the `&D_0015EBC0` / `&D_00153D20` address-constant materialization (lui hoisted into the
  `b` delay slot; lo re-added each loop iteration; the single-use `D_00153D20` split across
  the bnez + jal delay slots),
- the 9-arg register convention (a0-a3, t0-t3, stack arg at 0(sp)).

## What does NOT match (the blocker)

The epilogue (words 50-68) has a **consistent register-allocation shift** vs the original,
despite identical preceding code and (believed) identical C:

```
            original        mine
  idx load   v1              v0
  f (cmp)    v1              v0
  n = f*4    a0              v1
  Pbase      a1              a0
  P[f]       v0              v0
  Rbase      a0 (reused)     v1 (reused)
  0xFFC      a2              a1
```

8 non-relocation word diffs at indices 51,53,55,57,59,61,62,66. It reads as if the original
EGC had one extra register claimed before the epilogue (shift a0→a1→a2, v0→v1). The root
split is the epilogue's `lw <reg>, D_0015ECC0`: original picks v1 (the colder of v0/v1),
mine picks v0 (the hotter). Same free-register set, same last-use order (v0 last used at
word 44, v1 at word 39 in both) — yet different choice. This is an EGC register-allocation
behavior I cannot steer from C source.

## Things tried (all in /tmp/opencode/batch)

- `int* D_0015ECA0[2]`/`int[2]`/casts vs plain `int`+casts: no change.
- `buf`/`cnt` locals for the first call: no change (inlined away).
- Pointer temporaries (`int* p = P[f]; *p = 0;`), R-before-P store reorder: WORSE (frame
  grows to 0x80, extra saved regs, different epilogue).
- Dropping `D_0015ECC4 = 1` from the C: fixes the body SIZE (69 words, was 71) but the
  epilogue register shift remains.

## Dead tail (snd_UnkFunction_0012eaf0)

Original bytes at 0x12EAF0: `li v0,1; jr ra; sw v0,D_0015ECC4; nop` — the unreachable
image of the `D_0015ECC4 = 1` store. In a standalone EGC test the same 4-store epilogue
with NO saved registers DOES emit the 4th store dead after `jr ra` (help_msg-style
artifact), so the mechanism is real; but in the full function (with the 6 lq's) my EGC
keeps the 4th store alive. Because the body register shift is unresolved, the tail cannot
be regenerated to parity either. Orphan INCLUDE_ASM retained (supplies the bytes);
baseline build + `cmp` pass.

## Blocker

EGC 2.95.2 epilogue register allocation (f/n/Pbase/Rbase/0xFFC → v0/v1/a0/a1/a2) does not
match the original (v1/a0/a1/a0/a2) for byte-identical preceding code and equivalent C.
Not steerable from C source after many variants. Suspect a compiler-build/flag nuance in
EGC's register allocator, or a subtle original-C difference not yet identified. Revisit if
the EGC build/flags are revisited.
