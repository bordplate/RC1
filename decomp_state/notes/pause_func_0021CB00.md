# func_0021CB00 (vram 0x21CB00, file 0x11DA80, 48 bytes) - MATCHED 2026-09-08

## Behavior

Stores an 80% scaled copy of the sound-volume state and returns zero:

```cpp
D_0013E5A0 = *(int*)0x15EDF0 * 8 / 10;
return 0;
```

Ghidra does not define a function at 0x21CB00 and reports no xrefs to that
address. Its global xrefs and the adjacent `SoundOptionsMenu` show that
0x15EDF0 is one of the sound-option volume values and that the 0x13E598..
0x13E5AC group holds related scaled values. The generated assembly is the
source of truth for this otherwise unreferenced leaf.

## Codegen

A symbolic `.data` declaration for the input is behaviorally correct but does
not match: EGC keeps a separate address base in `v0` and schedules `li a0,10`
between the `lui` and `lw`. Reading through the constant-address cast emits the
original self-based load (`lui v1,0x16; lw v1,-0x1210(v1)`) and gives the exact
instruction order. The output remains a symbolic `.data` declaration so its
HI16/LO16 relocations produce the original `lui a1,0x14` and delay-slot store.

## Verification

- `func_0021CB00` is a 48-byte FUNC symbol at `.text+0x3B68`, with no
  `.NON_MATCHING` alias.
- Object disassembly matches all 12 original instructions after resolving the
  `D_0013E5A0` HI16/LO16 relocations.
- `cmp -n 48 -i 0x11DA80:0x11DA80 build/boot_elf.elf assets/boot_elf.elf`
  passes after a clean rebuild.
- `cmp build/boot_elf.elf assets/boot_elf.elf` passes after `make clean`,
  `make split`, and `make -j2`.
- Nonmatching count: 739 -> 738.
