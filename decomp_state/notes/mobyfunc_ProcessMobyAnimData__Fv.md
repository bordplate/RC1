# ProcessMobyAnimData__Fv (0x0020D1A8, file 0x10E128, 0x44 bytes)

## What it does

Stages the VU1 moby animation data for the next frame:
`FlushCache(0); FastMemCopy((void*)0x70003800, D_00165500, 0x800);
MobyAnimProc(*(int**)MOBY_ANIM_CHAIN_ADDRESS, D_0015F63C);`

- D_00165500 is a 0x800-byte RAM buffer (in .data) holding the current
  VU1 moby animation data; it is the source for the DMC region at
  0x70003800. Its first 0x80 bytes are also DMA'd as a VU1 header by the
  handwritten lighting pass at 0x234F98. The data producer is not
  decompiled yet, so the address name is retained.
- MobyAnimProc (0x00211728) is a handwritten entry point in
  code/_generated/game/mobyproc.s and is not cfront-mangled, hence
  `extern "C"`. It moves both arguments into FPU registers, saves the
  GPR state to D_001B7680, then walks the first argument as a linked list
  (`lw $at, 0($a0)` terminator test, func_0020E0E0 per node, func_001EE650
  for the next), waits on the VU1 status flag at 0x1000D000 (bit 0x100),
  and restores. Both parameters are therefore pointers.
- The two MobyAnimProc arguments are the VU1 swap-chain slot pointers
  maintained by VU1_initChain / VU1_swapChain (vuchain.cpp, still
  unmatched); the pointer at D_0015F63C equals the chain head pointer
  (at D_0015F638) minus 0x2000. The writers are unmatched, so
  D_0015F63C keeps its address name with an explanatory comment.
- D_0015F638 (the chain head pointer cell, .lit, written by
  VU1_swapChain) is referenced via the MOBY_ANIM_CHAIN_ADDRESS constant
  — see the codegen exception below. D_0015F638 is still pinned in
  config/symbols.txt because the generated nonmatching asm
  (VU1_swapChain__Fv.s) references it by name.
- 0x70003800 is a genuine DMC constant, matching the 0x70003A00 sibling
  constants in InitMobyClassDists / StashRestoreMobyClassDists. No symbol
  exists at 0x70003xxx in the original; naming it (a `.data` int symbol +
  cast) was probed and changes EGC's constant materialization and arg lui
  order — a 5-word diff at 0x20D1B8-CC — so the cast stays, documented.

## Codegen

Three different address forms are required for the three data
references:

- FastMemCopy p2 (D_00165500) must be the NAMED array extern
  `extern u8 D_00165500[];`. A constant cast `(u8*)0x165500` makes EGC
  set up a0 (0x70003800) first and a1 second, swapping the original
  `lui a1,0x16 / lui a0,0x7000` order; the named symbol emits a1 first
  and its low part (`addiu a1,a1,21760`) lands in the FastMemCopy jal
  delay slot.
- MobyAnimProc arg2 (D_0015F63C) must be a plain in-window scalar extern
  (declared `extern int* D_0015F63C;` — the value is a pointer), which
  gives the original GPREL16 load `lw a1,-30148(gp)` in the MobyAnimProc
  jal delay slot.
- MobyAnimProc arg1 must be a CONSTANT-ADDRESS load of 0x15F638
  (self-based `lui a0,0x16; lw a0,-2504(a0)`). MOBY_ANIM_CHAIN_ADDRESS is
  the documented constant form of that address (the linker symbol
  D_0015F638).

### Codegen exception for 0x15F638 (full probe record, 2026-09-13)

A user review rejected the original `*(int*)0x15F638` literal cast, so
every named-symbol form was probed (tools/decomp_probe.py, default flags
unless noted; all results below are 3-word diffs at 0x20D1D0/D4/DC unless
stated otherwise — base register $v0 instead of $a0, with the D_0015F63C
GPREL16 load hoisted from the delay slot up to 0x20D1D4):

- `extern int D_0015F638[];` + `D_0015F638[0]` -> two-register load.
- `extern u8 D_0015F638[];` + `*(int*)D_0015F638` -> two-register load.
- `extern int D_0015F638[] __attribute__((section(".data")));` + cast ->
  two-register load.
- `extern int D_0015F638 __attribute__((section(".data")));` +
  `*(int*)&D_0015F638` and + the value directly -> two-register load.
- Same set with -mno-gpopt -> two-register load.
- `-mno-split-addresses` + `int[]` + `[0]` -> the 0x15F638 load becomes
  the correct self-based pseudo (`lw $4, D_0015F638`) and D_0015F63C
  stays GPREL16 in the delay slot, BUT the FastMemCopy hi-load order
  flips to a0, a2, a1 (diff at 0x20D1B8/BC/C0): under the flag the array
  address becomes one `la` pseudo scheduled after the constant setups.
  Also failed with +(-fno-schedule-insns | -fno-schedule-insns2 | both),
  (void*) cast on the array arg, sized array, int array, +(-mno-gpopt).
- -O1 -> 6-word diff; -O0 -> 30-word diff.
- expert (GPT-6 Astra, one-shot) recommendation
  `register int* p asm("$4") = &D_0015F638; int v = *p;` -> EGC ignored
  the asm register constraint (still `lui $2; ...; lw $4,%lo($2)`).
- last-resort (GPT-5.6 Sol) recommendation: pointer-typed globals
  `extern "C" void MobyAnimProc(int*, int*); extern int* D_0015F638
  __attribute__((section(".data"))); extern int* D_0015F63C;
  MobyAnimProc(D_0015F638, D_0015F63C);` -> same two-register load; the
  `int* volatile` variant made it worse (8-word diff). The pointer
  prototype was kept regardless because it is the true signature (see
  What it does).

Conclusion: no named-symbol form or flag combination reproduces the
original self-based load while keeping the FastMemCopy argument order in
the same function; the constant-address load is the only matching form.
The address is named via the MOBY_ANIM_CHAIN_ADDRESS enum constant with
this explanation rather than a bare literal.

## Verification

- Probe (tools/decomp_probe.py): 68/68 bytes identical to
  code/_generated/matchings/game/mobyfunc/ProcessMobyAnimData__Fv.s.
- Object: all 17 words identical; R_MIPS_26 relocs resolve to the
  original jal targets (FlushCache 0x118A80, FastMemCopy 0x1F98D0,
  MobyAnimProc 0x211728).
- Clean full `make clean && make split && make -j4` + `cmp
  build/boot_elf.elf assets/boot_elf.elf`: identical (decomp-verifier
  PASS, 2026-09-13). Count 706 -> 705.
