# voBufIsFull__FP5VoBuf (code/game/movie/vobuf.cpp)

- Original: 20 bytes (`0x14`) at file offset `0x13E178` in `assets/boot_elf.elf`,
  vram `0x0023D1F8`. Object-relative `.text+0x68` in `vobuf.o`.
- Semantics: ring-buffer fullness test — returns `count == capacity` for a
  `VoBuf` (`count` at offset `0xC`, `capacity` at offset `0x10`).
- Original instruction sequence (file-byte order):
  ```
  lw     $v1, 0x10($a0)   # capacity -> $v1
  lw     $v0, 0xC($a0)    # count    -> $v0 (volatile)
  xor    $v0, $v0, $v1
  jr     $ra
     sltiu $v0, $v0, 0x1  # delay slot: result = ((count ^ capacity) < 1)
  ```
- Unsigned `a == b` idiom: EGC compiles `==` on unsigned values to
  `xor; sltiu r,r,1`. The xor must leave the result in `$v0` (return value),
  so count goes to `$v0` and capacity to `$v1`; the scheduler puts `sltiu`
  into the `jr $ra` delay slot.
- Ghidra decompile of `FUN_0023d1f8`:
  `return *(int *)(param_1 + 0xc) == *(uint *)(param_1 + 0x10);`

## C candidate

```cpp
extern "C" int voBufIsFull__FP5VoBuf(VoBuf* self) {
    return self->count == self->capacity;
}
```

Uses the existing `volatile u32 count` field from the voBufReset fix (forces
a real load and keeps the two loads from being merged/reordered). Standalone
EGC 2.95.2 `-G8 -O2 -ffast-math -fno-exceptions -Wa,-EL` test compile of this
body produced the exact original sequence on the first try — no operand
reordering or delay-slot surprises, unlike the voBufReset zero-store pair.

## Verification (mechanical)

- `make` builds; `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- `objdump -d build/code/game/movie/vobuf.o`: symbol
  `voBufIsFull__FP5VoBuf` at `.text+0x68`, size `0x14`, words
  `8c830010 8c82000c 00431026 03e00008 2c420001` — identical to the original
  slice at file offset `0x13E178`.
- `objdump -r build/code/game/movie/vobuf.o`: zero relocations in the file,
  so no GP/data relocation risk.
- Side benefit: a sibling function in vobuf.o (the push function at
  `.text+0x104`) now resolves to `jal 68 <voBufIsFull__FP5VoBuf>` instead of
  an absolute literal; the fixed-address linker script places it at the same
  original address, which is why full ELF parity still holds.

## Remaining vobuf.cpp nonmatchings (next candidates)

- `voBufDelete__FP5VoBuf` — bare no-op (`jr $ra; nop`); empty C function body.
- `voBufIncCount__FP5VoBuf`, `voBufDecCount__FP5VoBuf` — count guard + inc/dec
  (blez/bgtz + addiu in delay slot; watch volatile store scheduling).
- `voBufGetData__FP5VoBuf`, `voBufGetTag__FP5VoBuf` — the pop pair at
  vram 0x23D2D8 with the `beqzl -> break 7` capacity guard.
- `voBufCreate__FP5VoBufP6VoDataP5VoTagi` — multi-arg setup, larger.
