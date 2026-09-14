# Camera_commitPendingTransform__Fv (code/game/camera.cpp)

- Original: 56 bytes (`0x38`) at vram `0x1EC868`, symbol
  `Camera_commitPendingTransform__Fv` (C++ `()`). Renamed from
  `func_001EC868`; the address is pinned in `config/symbols.txt`.
- Semantics: commit the staged camera transform when a mode switch is pending.
  Reads the mode byte at `camTransState+0x02`; when nonzero, copies the two
  16-byte *pending* quads over the two *active* quads:
  `*(base+0x50) = *(base+0xC0)` and `*(base+0x60) = *(base+0xD0)`.
  `base` = `camTransState` = `0x1871B0` (an 0xE0-byte block, section `.data`).
  The caller `func_001EC8A0` (0x1EC8A0) uses the same four offsets, so the
  block holds the active (+0x50/+0x60) and pending (+0xC0/+0xD0) camera
  transforms. Declared as `struct CameraTransState` with four 16-byte
  `CameraQuad` fields.

## Key codegen finding: R5900 `lq`/`sq` are 128-bit, `ld`/`sd` are 64-bit

The two copies move 16-byte blocks. EGC lowers a 64-bit `long`/`long
long`/`double` to `ld`/`sd` (opcodes 0x37/0x3F) but the original uses `lq`/`sq`
(opcodes 0x1E/0x1F). The 128-bit type is what selects `lq`/`sq`:

```cpp
typedef unsigned int CameraQuad __attribute__((mode(TI)));  // sizeof == 16
```

A `mode(TI)` copy emits `lq`/`sq`. (Confirmed with a standalone EGC probe;
`-mips1..4` and `-mabi=eabi/o64` do NOT switch a 64-bit type to `lq`/`sq`.)
This is the project's first 128-bit-data target; the original has 6398 data
`lq`/`sq` accesses, so the pattern is widespread (the sibling camera function
at 0x1EC8A0 uses it too).

## Why inline asm (not natural C)

Even with `CameraQuad` fields, EGC emits the copies with the offsets FOLDED
into the `lq`/`sq` (`lq $v0, 192($a0)`) and a `nop` in the `beqz` delay slot.
The original instead MATERIALIZES each field address in its own `addiu`
(base in `$a2`, offset-0 `lq`/`sq`) and parks the first `addiu` in the `beqz`
delay slot. No C form (field copy, pointer deref, volatile, explicit pointers)
nor flag (`-O1`, `-fno-schedule-insns[2]`, `-fno-inline`) reproduces that —
EGC always folds. The exact bytes are therefore emitted with a `.set
noreorder` inline-asm block (same mechanism as `INCLUDE_ASM`):

- `.set noreorder` is what keeps the `addiu` in the `beqz` delay slot; without
  it the R5900 assembler inserts a nop there and the function grows to 60 bytes,
  shifting all subsequent `.text` and breaking whole-ELF parity.
- The asm references `camTransState` by name so the `%hi`/`%lo` relocs resolve
  to `0x1871B0` (EGC's own global accesses use `%hi`/`%lo` and numeric register
  names `$2`/`$3`/`$4`/`$5`/`$6`; `%hi0` and symbolic `$v0` names are rejected
  by the EE assembler).
- Clobber list uses the `$`-less names `"2" "3" "4" "5" "6"` and the
  `: : :` (spaced) operand separator; this EGC build rejects `:::` and
  `"$v0"`-style clobbers.

Verified: function bytes identical to the original (objdump diff), clean
`make clean && make split && make`, and `cmp build/boot_elf.elf
assets/boot_elf.elf` passes.
