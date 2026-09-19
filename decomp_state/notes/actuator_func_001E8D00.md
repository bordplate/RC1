# func_001E8D00 (vram 0x1E8D00, file 0xE9C80, 8 bytes) — MATCHED 2026-09-19

Not a function: the first 8 bytes of the entire resident `.text` section are
zero (two nops), sitting before the first real function of the first
`.text` object, `actuator_CalcPower` (0x1E8D08). spimdis split the 8-byte
head into a phantom 4-byte "function" (0x1E8D00..0x1E8D03) plus one
trailing nop after its endlabel (0x1E8D04).

## Evidence

- `.text` starts at file 0xE9C80 / vram 0x1E8D00 (section table); raw bytes
  `00 00 00 00 00 00 00 00` followed by `40 ff bd 27` =
  `addiu sp,sp,-0xC0`, the `actuator_CalcPower` prologue at 0x1E8D08.
- Ghidra has no function at 0x1E8D00; the only xref is the DATA row of the
  ELF section header table (the section itself), i.e. no code reference.
- No `jal`/`j`/relative-branch target and no absolute data word points into
  0x1E8D00..0x1E8D07 (same deadness reasoning as the dead-tail notes; the
  region is the section head, so nothing can fall into it from below either).
- `core.text` (the other resident code section, 0x112380) starts clean with
  code (`addiu sp,sp,-0x10`), so the head padding is specific to the
  `.text` section's first object — the original actuator object's leading
  bytes, not a global linker artifact.
- The neighbouring `lvl.vtbl` / `lvl.camvtbl` / `lvl.sndvtbl` rodata sections
  (0x1E8B80..0x1E8C88) are sentinel vtables (`0xffffffff, 0x0, ...`), not
  actuator pointers, so the 8 zero bytes are not a recognizable data table;
  they behave as plain padding and are supplied as nops.

## Replacement (code/game/actuator.cpp)

The `INCLUDE_ASM(..., func_001E8D00)` (which emitted the 2 nops via the
generated `.s`) was replaced by the project's standard padding idiom
(same as stash.cpp, boot.cpp, and actuator.cpp's own post-texResetCursor
nops):

```cpp
// The original .text section starts with 8 padding bytes (two nops) before
// the first actuator function; spimdis split them into a phantom 4-byte
// "function" (func_001E8D00) plus one trailing nop. Nothing jumps here.
asm("nop");
asm("nop");
```

Nothing references the `func_001E8D00` label (the linker script places whole
object sections, `actuator.o(.text)`, not individual symbols), so dropping
the `glabel` is safe.

## Verification

- actuator.o `.text` = 0x468 (unchanged); bytes identical to original
  0x1E8D00..0x1E9167 except the pending `R_MIPS_HI16 ActuatorWave` at
  object offset 0x30 (resolves to 0x16 at link, as before).
- Clean `make clean && make split && make -j2` +
  `cmp build/boot_elf.elf assets/boot_elf.elf` = PARITY OK.
- `decomp_status --count` 684 -> 683. Splat now writes
  `matchings/game/actuator/func_001E8D00.s` (no active INCLUDE_ASM remains
  for it).
