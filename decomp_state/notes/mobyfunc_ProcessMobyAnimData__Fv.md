# ProcessMobyAnimData__Fv (0x0020D1A8, file 0x10E128, 0x44 bytes)

## What it does

Stages the VU1 moby animation data for the next frame:
`FlushCache(0); FastMemCopy((void*)0x70003800, D_00165500, 0x800);
MobyAnimProc(*(int*)0x15F638, D_0015F63C);`

- D_00165500 is a 0x800-byte RAM buffer (in .data) holding the current
  VU1 moby animation data; it is the source for the DMC region at
  0x70003800. Its first 0x80 bytes are also DMA'd as a VU1 header by the
  handwritten lighting pass at 0x234F98. The data producer is not
  decompiled yet, so the address name is retained.
- MobyAnimProc (0x00211728) is a handwritten entry point in
  code/_generated/game/mobyproc.s and is not cfront-mangled, hence
  `extern "C"`.
- The two MobyAnimProc arguments are the VU1 swap-chain slot offsets
  maintained by VU1_initChain / VU1_swapChain (vuchain.cpp, still
  unmatched); the value at D_0015F63C equals the value at 0x15F638 minus
  0x2000. The writers are unmatched, so D_0015F63C keeps its address
  name with an explanatory comment. D_0015F638 itself stays a constant
  cast (codegen, below).
- 0x70003800 is a genuine DMC constant, matching the 0x70003A00 /
  0x70003xxx sibling constants in InitMobyClassDists /
  StashRestoreMobyClassDists.

## Codegen

Three different address forms are required for the three data
references:

- FastMemCopy p2 (D_00165500) must be the NAMED array extern
  `extern u8 D_00165500[];`. A constant cast `(u8*)0x165500` makes EGC
  set up a0 (0x70003800) first and a1 second, swapping the original
  `lui a1,0x16 / lui a0,0x7000` order; the named symbol emits a1 first
  and its low part (`addiu a1,a1,21760`) lands in the FastMemCopy jal
  delay slot. The 0x165500 hi/lo split uses the signed split
  (0x16 / +21760) either way.
- MobyAnimProc arg1 must be the constant cast `*(int*)0x15F638`. 0x15F638
  is inside the gp window, so a named scalar extern compiles to a
  GPREL16 load, but the original uses an absolute self-based
  `lui a0,0x16; lw a0,-2504(a0)`.
- MobyAnimProc arg2 (D_0015F63C) must be the plain scalar extern
  `extern int D_0015F63C;`, which gives the original in-window GPREL16
  load `lw a1,-30148(gp)` in the MobyAnimProc jal delay slot.
- MobyAnimProc must be declared `void MobyAnimProc(int, int)`: declaring
  the first parameter `int*` makes EGC fail the build with
  "passing `int' to argument 1 of `MobyAnimProc(int *, int *)' lacks a
  cast" (int-to-pointer conversion is an error, not a warning).

Confirmed general rule (third data point after UpdateTieTextures and the
tiefunc D_ globals): plain ARRAY externs compile to absolute %hi/%lo
relocations regardless of gp-window position, while plain in-window
SCALAR externs compile to GPREL16.

## Verification

- Probe (tools/decomp_probe.py): 68/68 bytes identical to
  code/_generated/nonmatchings/game/mobyfunc/ProcessMobyAnimData__Fv.s.
- Object: all 17 words identical; R_MIPS_26 relocs resolve to the
  original jal targets (FlushCache 0x118A80, FastMemCopy 0x1F98D0,
  MobyAnimProc 0x211728).
- Clean full `make clean && make split && make -j4` + `cmp
  build/boot_elf.elf assets/boot_elf.elf`: identical (decomp-verifier
  PASS, 2026-09-13). Count 706 -> 705.
