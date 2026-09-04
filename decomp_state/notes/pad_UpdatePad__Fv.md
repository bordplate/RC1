# UpdatePad__Fv (code/game/pad.cpp) — MATCHED 2026-09-04

Single-call wrapper, 0x20 bytes at vram `0x00217A10` (file offset `0x118990`):
calls `UpdatePad(PAD&)` (binary symbol `UpdatePad__FR3PAD`, vram `0x2170C8`)
with the global pad instance `D_0013C940` as the argument, returns its result.

## Identity / context

Ghidra `FUN_00217a10`: `void FUN_00217a10(void) { FUN_002170c8(0x13c940); return; }`
(`FUN_002170c8` = `UpdatePad__FR3PAD`, the still-`INCLUDE_ASM` `PAD` update
method). Four UNCONDITIONAL_CALL xrefs: `0x001e97ec`, `0x001eba08`, `0x00232670`,
`0x0023a4c8` — called as the no-arg "poll the resident pad state" entry point
from game update/camera/sound/movie paths. The callee is old-cfront-mangled
`UpdatePad(PAD&)` (free function taking `PAD&`, NOT a class method: no
`N<class>` component), so the wrapper is a no-arg overload `UpdatePad()` of the
same name.

`D_0013C940` is the shared pad/stream global (declared `StreamState` in
`code/game/stream.cpp`, same symbol also the callee argument here); its link
value is fixed by `SCUS_971.99.ld` (`D_0013C940 = 0x13c940`). It lies far
outside the gp window (gp=0x166C00).

## Original bytes (LE words)

```
addiu   $sp, $sp, -0x10          # f0ffbd27
lui     $a0, 0x14                # 1400043c   %hi(D_0013C940) (rounded up)
sq      $ra, 0($sp)              # 0000bf7f
jal     UpdatePad__FR3PAD        # 325c080c   (0x2170C8 >> 2)
addiu   $a0, $a0, -0x36C0        # 40c98424   %lo(D_0013C940), signed
lq      $ra, 0($sp)              # 0000bf7b
jr      $ra                      # 0800e003
addiu   $sp, $sp, 0x10           # 1000bd27   (delay slot)
```

## Replacement (code/game/pad.cpp, old INCLUDE_ASM line 9)

Pure C++ (no `extern "C"` hand-mangled names; EGC's old cfront mangling
produces the exact binary symbols):

```cpp
struct PAD;

extern PAD D_0013C940;

void UpdatePad(PAD& self);

void UpdatePad(void) {
    UpdatePad(D_0013C940);
}
```

- `void UpdatePad(void)` -> `UpdatePad__Fv` (replaces the placeholder).
- `void UpdatePad(PAD& self)` -> `UpdatePad__FR3PAD` (declaration only; the
  definition stays the `INCLUDE_ASM` above it in the same file).

## Codegen findings

- The argument MUST be a symbol address, not a literal. Passing the constant
  `0x13C940` compiles to `lui $a0,0x13; ori $a0,$a0,0xC940` (EGC's constant
  decomposition), which does NOT match the original's
  `lui $a0,0x14; addiu $a0,$a0,-14016` %hi/%lo symbol pair. Referencing
  `D_0013C940` emits `R_MIPS_HI16`/`R_MIPS_LO16` that resolve to the original
  words. Same symbol-vs-constant lesson as the `func_0021CAE0` / menu-family
  notes.
- `extern PAD D_0013C940;` with `PAD` an INCOMPLETE type needs no
  `section(".data")` attribute: EGC emits absolute HI16/LO16 (not the
  small-data GPREL16 that a complete `extern int` would). It coexists cleanly
  with stream.cpp's `extern StreamState D_0013C940 __attribute__((section(".data")))`
  declaration (per-TU declarations; link value from the .ld assignment).
- Schedule: 0x10 frame with `sq/lq $ra` at `0(sp)`; the argument `lui` lands
  before the `sq $ra`, the `addiu %lo` in the `jal` delay slot, epilogue
  `lq; jr; <addiu $sp>`. EGC emits `R_MIPS_JMPADDR` (EABI) for the `jal`; the
  PS2 ld resolves it to the original's absolute `target>>2` word (verified
  again by this match; same pipeline behaviour as every earlier jal-containing
  match).
- `void` return type is byte-safe: the callee's `$v0` is passed through with no
  extra register traffic either way (Ghidra types both void).

## Verification (mechanical)

- `build/code/game/pad.o`: `UpdatePad__Fv` plain `T` at `.text+0x948`,
  st_size `0x20`, no `.NON_MATCHING` alias. Only relocations in its range:
  `R_MIPS_HI16`+`R_MIPS_LO16` vs `D_0013C940`, `R_MIPS_JMPADDR` vs the
  `.text` section symbol (target `UpdatePad__FR3PAD` at `.text+0`).
- Object bytes with relocs resolved == original slice
  `f0ffbd27 1400043c 0000bf7f 325c080c 40c98424 0000bf7b 0800e003 1000bd27`.
- Link map: `UpdatePad__FR3PAD` @ `0x2170c8`, `UpdatePad__Fv` @ `0x217a10`.
- Clean verification: `make clean && make split && make -j2` then
  `cmp build/boot_elf.elf assets/boot_elf.elf` passes (byte-for-byte).
- `decomp_status --count`: 819 -> 818.
