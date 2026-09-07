# func_001F4248 (0x1F4248, 52 bytes) — BLOCKED (EGC guard-load RTL split)

`code/game/draw.cpp:65`. Semantics:

```c
if (*(int*)0x15F618 == 0) {   // D_0015F618, section .lit4 (writable)
    func_001FB368();          // GS packet emitter, void, no args (nop ds)
    D_0015F434 = 0x7F;        // .lit4, gp-relative sw in 2nd jal ds
    DrawDebugProfiler();      // void, no args
}
```

Caller: FUN_00230ee8 (debug-display mode switch: mode 0/8 calls this setup;
mode 3/7 sets the 0x7F flag + DrawDebugProfiler directly). D_0015F434 is the
debug-layer bitmask consumed by DrawDebugProfiler (0x1F39D0).

Original 13 words:

```
[0] 27bdfff0 addiu sp,-0x10
[1] 3c020016 lui   v0,%hi(D_0015F618)
[2] 8c42f618 lw    v0,%lo(D_0015F618)(v0)   # base v0, value v0
[3] 14400007 bnez  v0,.L
[4] 7fbf0000 sq    ra,0(sp)                 # prologue save in the bnez ds
[5] jal func_001FB368
[6] nop
[7] 2402007f addiu v0,0,0x7F
[8] jal DrawDebugProfiler
[9] af828834 sw   v0,D_0015F434             # R_MIPS_GPREL16
[10] 7bbf0000 lq  ra,0(sp)
[11] 03e00008 jr   ra
[12] 27bd0010 addiu sp,0x10                 # jr ds
```

## The wall: two source families, neither unifies load form + schedule

EGC 2.95.2 reaches reload with two structurally different RTL shapes for the
guard load, and each family gets exactly one of the two required properties
right (probes `decomp_state/probes/draw_func_001F4248{,_v2}.cpp`):

### A. Constant-based address (`*(int*)0x15F618`, or folded local pointer)

The load reaches reload as ONE indivisible RTL memory op
`(set (reg:SI v0) (mem:SI (const_int 0x15f618)))`; the `lui` only appears
when the assembler splits it, i.e. AFTER scheduling. Result (14 words):

```
addiu sp; sq ra; lui v0; lw v0; bnez v0;
<ds> lq ra          # epilogue restore pulled into the bnez ds
jal; nop; li v0,0x7F; jal; <ds> sw; lq ra; jr; <ds> addiu sp
```

Load form correct (lui v0 / lw v0 / addiu sp first) but the prologue keeps
the `sq ra` and the epilogue `lq ra` is legally hoisted into the branch ds
(common to both paths) → +1 word, 2 wrong words.

### B. Symbol-based address (`extern "C" int D_0015F618
__attribute__((section(".lit4")));`, also `int[]` / struct member / `&`-cast /
static / `sym+0` / local `int x=sym`)

The guard is two RTL ops before reload (`r117 = high(sym)`,
`r118 = [r117 + low(sym)]`); global allocation fixes r117→v0, r118→v1. Result
(13 words):

```
lui v0,sym; addiu sp; lw v1,off(v0); bnez v1;
<ds> sq ra          # correct: save in the branch ds, no redundant lq
jal; nop; li v0,0x7F; jal; <ds> sw; lq ra; jr; <ds> addiu sp
```

Schedule correct but the `lui` is emitted ABOVE `addiu sp` and the value
lands in v1 (words [0]/[1] swapped, `lw`/`bnez` use $3 vs $2).

### Plain in-window symbol (`extern "C" int D_0015F618;`)

`lw v0,off(gp)` single-instruction GP-relative load (also with `const int`,
`int[1]`) → 12 words. Wrong load form; the original is absolute lui/lw, so
Insomniac did not use a plain small-data extern for this guard.

## Exhausted matrix (this session + last-resort GPT-5.6 Sol)

- Source forms: constant cast, local pointer `int* p=(int*)ADDR; *p`, local
  `int x`, section-attr symbol, `int[]` / `int[1]` / `const` extern, struct
  member, `*(int*)&sym`, `sym+0`, `0 == sym`, `!sym`, static function,
  `__attribute__((aligned(8)))`, fixed-register locals, volatile variants,
  reassigning a pointer with the loaded value, inline-asm `lui`.
- Flags: `-fno-schedule-insns`, `-fno-schedule-insns2`, both,
  `-mno-split-addresses`, `-G0`, `-ffixed-v1`, `-fforce-addr`,
  `-fforce-mem`, `-frerun-cse-after-loop`, `-frerun-loop-opt`,
  regmove/optimize-register-move/GCSE/expensive-opts/peephole/function-CSE/
  caller-saves disabled, `-O0/-O1/-O3`, C vs C++ (cc1 vs cc1plus),
  callee return-type matrix (void/int for both calls).
- Notable flips: `-fno-schedule-insns2` (or both) on form B flips it to the
  form-A 14-word shape with v0 registers (still wrong sq placement);
  `-O1` = form A; `-O3` = form B default; `-ffixed-v1` pushes the load to a0;
  reserving all caller-saved alternatives spills to s0 + bigger frame.

## Positive controls (same EGC)

- `audioDecSend` (code/game/movie/audiodec.cpp:34) matches with EXACTLY the
  goal schedule (`sq ra` in the branch ds, no redundant lq) — but its guard
  is a single `lw v0,0(a0)` from a pointer parameter (no lui to schedule).
- `memcard_Init` (code/game/memcard.cpp:19) matches WITH the redundant-lq
  pattern: its branch is on a CALL result, and the original itself carries
  the redundant `lq ra` in the ds. func_001F4248's branch is on a LOAD, so
  neither matched control covers this shape.

## Conclusion

The goal stream requires the symbol-family schedule (sq in branch ds, no
redundant lq) with the constant-family load materialization (addiu sp first,
lui v0, value in v0). Our EGC build couples schedule to address kind at the
RTL level: symbol addresses are independent `high`/`[base+low]` ops that get
hoisted + v1, constant addresses are one indivisible mem op that keeps the
prologue sq + pulls the lq into the ds. No source form or flag in this build
decouples them. Last-resort GPT-5.6 Sol (2026-09-07) reproduced the RTL
evidence above and found no escape. Retain the INCLUDE_ASM fallback.

Sister risk: any function whose guard/test load is an absolute lui/lw to a
non-small-data symbol AND whose body contains a call before the common
epilogue may hit the same split. func_001EC868 (camera) is separately
suspect for a different reason (lq/sq 64-bit DATA copies — research probes
in decomp_state/probes/camera_func_001EC868*.cpp).
