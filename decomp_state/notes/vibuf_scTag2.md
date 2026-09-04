# scTag2 (code/game/movie/vibuf.cpp)

Blocked 2026-09-04: EGC 2.95.2 SN v2.73a cannot reproduce the original 64-bit
shift + per-operand zext codegen; it folds everything to 32-bit.

## Semantics (Ghidra + asm)
`void scTag2(u64* p, u32 a, u32 b, u32 c)` — writes a 64-bit DMA tag:
```
*p = (u64)a | ((u64)b >> 4) | (u64)c;
```
4-arg leaf, no frame, no calls. Callers (in nonmatching viBuf fns) pass
`(tagPtr, dmaAddr, ctd, 0x80)`. Result is always < 2^32 (all operands 32-bit),
so the upper 32 bits of the stored tag are zero.

## Original asm (9 words, 0x0023BC20)
```
dsll32 $6,$6,0     ; zext b
dsll32 $5,$5,0     ; zext a
dsrl   $6,$6,4     ; 64-bit shift b by 4   <-- dsrl (func 0x1A)
dsll32 $7,$7,0     ; zext c
or     $5,$5,$6     ; 32-bit or  (func 0x25)
dsrl32 $7,$7,0     ; zext c (redundant)
or     $5,$5,$7     ; 32-bit or
jr     $ra
   sd   $5,0($4)
```

## What my EGC 2.95.2 emits (best variant w1, 10 words)
```
dsll32 a2,a2,0     ; zext b
dsll32 a1,a1,0     ; zext a
dsrl32 a1,a1,0     ; zext a (redundant)  <-- extra
dsll32 a3,a3,0     ; zext c
dsrl32 a2,a2,4     ; 32-bit shift b      <-- dsrl32 (func 0x1E), NOT dsrl
dsrl32 a3,a3,0     ; zext c (redundant)
or     a1,a1,a2
or     a1,a1,a3
jr     ra
   sd   a1,0(a0)
```

## Root cause
The original keeps the shift at 64-bit (`dsrl`) on a zext'd value and zext's
each operand up front. EGC 2.95.2 proves every operand is < 2^32 (they are
32-bit params that get zext'd), so it:
  - folds `(u64)b >> 4` to a 32-bit shift (`srl`/`dsrl32`), and
  - defers a single zext to the end of the or chain.
This folding happens at every -O level (verified -O0/-O1/-O2). There is no
source formulation that makes EGC 2.95.2 emit the 64-bit `dsrl` on a value it
knows is < 2^32 — the value would have to be a genuine 64-bit quantity, which
would then NOT be zext'd (contradicting the original's leading `dsll32`s).

## Attempts (all fold to 32-bit)
`(u64)a|((u64)b>>4)|(u64)c`; `(u64)(a|((b>>4)|c))`; explicit `u64` temps
(`u64 bb=b,aa=a,cc=c; *p=aa|(bb>>4)|cc`); `(u64)a|(u64)(b>>4)|(u64)c`;
`u64 t=(u64)b; t>>=4; t|=(u64)a; t|=(u64)c);` `(a|c)|(b>>4)`. -O0/-O1/-O2.

## Note
Same class of blocker as videodec_func_0023D0E0: the original EGC build kept
a 64-bit op (or a specific schedule) that EGC 2.95.2 SN v2.73a strength-reduces
/ reschedules. Keep INCLUDE_ASM.
