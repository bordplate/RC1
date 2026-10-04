# func_001FBAB0 (code/game/framebuf.cpp) — dead-tail ghost, 2026-10-04

`func_001FBAB0` @ 0x1FBAB0 is a 4-byte dead-tail fragment (the first active
framebuf target in source order), not a function:

- Bytes: `lw v0,20(v1)` (word 0x8C620014) at 0x1FBAB0 + alignment nop at
  0x1FBAB4, 8-aligned before `mode_freezeInit` @ 0x1FBAB8.
- Parent: func_001FB8F0 (0x1C0, AA-pass inline GS display-environment packet
  builder, blocked 2026-10-04). Its epilogue ends `jr ra; addiu sp,sp,128` at
  0x1FBAAC; the fragment sits right after, in the 8-aligned gap.
- Deadness: `tools/deadness_scan.py 0x1FBAB0` → 0 reference(s) (no jal/j/
  branch target, no 32/64-bit data word); no Ghidra function at the address.
- Shape: AGENTS.md dead-tail family "one instruction + nop" (this one is a
  stray value load into v0 after `jr ra`, not a stack deallocate).

Handling: per the dead-tail policy the placeholder was replaced in
framebuf.cpp with a byte-preservation `asm()` block (actuator.cpp pattern:
`nonmatching func_001FBAB0, 0x4` + glabel + exact `.word`s), which keeps the
glabel and the `.NON_MATCHING` tracking symbol in framebuf.o and emits the
exact original bytes. Not a decompilation target and not a blocker.

Verified: make + `cmp build/boot_elf.elf assets/boot_elf.elf` byte-identical;
object shows `func_001FBAB0` (T) and `func_001FBAB0.NON_MATCHING` (T);
nonmatching count 577 → 576.
