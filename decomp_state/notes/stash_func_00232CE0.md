# func_00232CE0 (vram 0x232CE0, file 0x133C60, 28 bytes) — MATCHED 2026-09-04

Stash-module SIF/IOP streaming init. One-statement wrapper that calls the
SCE-lib function `func_0011AB20` (in `code/_generated/sce/lib.s`, 0x19C bytes)
with argument 0:

```
addiu sp,sp,-0x10
sq    ra,0(sp)
jal   func_0011AB20
daddu a0,zero,zero   ; delay slot (word 0x0000202D, shows as "move a0,zero")
lq    ra,0(sp)
jr    ra
addiu sp,sp,0x10     ; delay slot
```

## Identity / context

Game-side "stash" = the IOP stash used for VAG/audio streaming. Module layout
(code/game/stash.cpp):

- 0x232CE0 this fn — one-shot SIF/IOP streaming setup (idempotent; callee
  guards on D_0012FC08).
- 0x232D00 — loads + starts the IOP module (PRG at D_001DD1A8 via
  func_0011AFF8/func_0011B3B8), then gets its data via func_0011B1C8 and
  zeroes the ring-buffer slot table at D_001DD1D0..0x1DD1DC (64 slots).
- 0x232E40 Stash_SendData / 0x232F20 — push chunks through the ring buffer
  with func_0011B1C8(0x1DD1A8, 1, ...).
- 0x233038 — slot-size getter over D_001DD1DC.

Callee func_0011AB20 (Ghidra): under lock func_0011D660/func_0011D6A8, if
D_0012FC08 set -> unlock, return. Else set D_0012FC08=1, func_0011A480(),
build the IOP DMA config struct at D_00156800 (three IOP RAM regions
0x20155000/0x20155800/0x20156000, 0x20 words each), register four SIF DMA
handlers via func_0011A738(ch 0x80000008/9/A/C, IOP code 0x11ADE0/0x11AF48/
0x11B138/0x11AE98, 0x156800), then poll sceSifGetReg(0x80000002): if not ready
set D_0015504C=1, SIF register transfer func_0011A8C8(0x80000002, 0x155040,
0x10, 0, 0, 0), spin on func_0011A458(0), sceSifSetReg(0x80000002, 1).
Callee ignores the argument (no a0 read in its body), but the original source
passed 0, hence the `daddu a0,0,0` in the delay slot.

Callers of func_00232CE0 (both bare `jal`, no args):
- 0x1EB900 in Transition_DoTransition__Fv (immediately before jal
  func_00232D00, the IOP-module load).
- 0x20197C in InitOnce__Fv.

## Codegen findings

- EGC -O2, zero-arg C function whose only statement is a call passing literal
  0: emits the 0x10 frame with `sq/lq $ra` at 0(sp) (same frame shape as
  matched func_001F6250), and hoists the argument load `daddu $4,$0,$0`
  (word 0x0000202D — a true DADDU, which objdump prints as `move a0,zero`)
  into the `jal` delay slot. No stray nops; epilogue is the standard
  `lq; jr; <ds addiu>`.
- The callee MUST be declared with a parameter (e.g. `int`) in the extern
  prototype: a `(void)` prototype suppresses the argument setup and the
  delay slot would come out as `nop`, breaking the match.

## Source

Replaced the INCLUDE_ASM in code/game/stash.cpp with:

```cpp
extern "C" void func_0011AB20(int param_1);

extern "C" void func_00232CE0(void) {
    func_0011AB20(0);
}
```

Kept the original `func_00232CE0` symbol name (project convention for C
symbols; final ELF is stripped so the name does not affect the binary; cf.
pause_func_002223D8.md). Semantic name: stash SIF/IOP streaming init
(a.k.a. Stash_Init).

## Verification

- Standalone EGC object: 8 words
  `f0ffbd27 0000bf7f <jal reloc> 2d200000 0000bf7b 0800e003 1000bd27`.
- Final image slice file 0x133C60..0x133C7C (28 bytes) built == original:
  `f0ffbd27 0000bf7f c86a040c 2d200000 0000bf7b 0800e003 1000bd27`.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- decomp_status --count: 831 -> 830.
