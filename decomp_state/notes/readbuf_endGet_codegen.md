# readBufEndGet__FP7ReadBufi (vram 0x23BA20, file 0x13C9A0, 36 bytes)

Semantics (Ghidra FUN_0023ba20): `self->count = old - min(old, n)` with SIGNED
comparison; `old = *(self+0x50004)` (ReadBuf.count). Not yet matched.

## Original body (target words)

```
3c020005  lui   v0, 0x5           # base = self + 0x50000 (v0 as hi temp)
00822120  addu  a0, a0, v0
8c820400  lw    v0, 4(a0)         # old = count
0040302d  daddu a2, v0, $0        # a2 = old (spill copy)
00a2182a  slt   v1, a1, v0        # n < old  (SIGNED)
00a3100b  movn  v0, a1, v1        # v0 = min(n, old)  [select into else-operand reg]
00c23023  subu  a2, a2, v0        # a2 = old - min
03e00008  jr    ra
ac860400  sw    a2, 4(a0)         # count = old - min
```

## Findings (2026-09-04)

- EGC 2.95.2 with project flags (-G8 -O2 -ffast-math -fno-exceptions) reproduces
  the exact instruction STRUCTURE from this shape:
    `self->count = self->count - ((n < self->count) ? n : self->count);`
  (field referenced ~4x so CSE keeps one load but allocation keeps the spill).
  Result with `int count` field: lui v1 / addu a0,a0,v1 / lw v0,4(a0) /
  move v1,v0 / slt a2,a1,v0 / movn v0,a1,a2 / subu v1,v1,v0 / jr ra / sw v1.
- Remaining delta is purely a register-allocation tiebreak: my compile keeps the
  copy in $v1 (cmp in $a2); original keeps the copy in $a2 (cmp in $v1, hi temp
  in $v0). ~70 source-shape variants tried (ternary/if-assign orders, named
  temps, `-=`, pointer-to-field, casts, unsigned fields) all land on the mirror
  allocation or on an 8-word body without the spill. No flag change found that
  flips it; per AGENTS.md the original compiler flags may still differ subtly.
- Signedness evidence: target uses SIGNED slt; sibling readBufEndPut (0x23B990)
  uses signed `slt` for its n<free min and signed `div` for the wrap-around
  modulo, while u32-field candidate compiles emit sltu/divu. The original
  ReadBuf almost certainly used plain `int` fields (or explicit signed casts);
  readbuf.cpp currently declares putPos/count/capacity as u32. Switching them to
  int is codegen-invisible for the already-matched readBufCreate (stores only).
- readBufEndPut next-step notes: C shape
  `if (capacity > 0) { free = capacity-count; if (n>free) n=free; putPos+=n;
  count+=n; putPos %= capacity; }` compiles with EGC to div+mfhi (EGC emits the
  remainder via hi-half, no mult/sub), matching the original's modulo idiom, but
  guard placement/registers differ (original guards the whole body right after
  the three loads with beql cap,$0 + `break` delay slot; my compile restructures).

Status: not committed. Candidate shapes and word dumps are in
/tmp/opencode/readbuf_test (fuzzer.sh, fuzz2-5.sh, ep.cpp, ep2.sh) if that dir
still exists; regenerate from this note otherwise.
