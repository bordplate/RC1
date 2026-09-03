# voBufIsEmpty (code/game/movie/vobuf.cpp)

- Original: 12 bytes (`0xC`) at file offset `0x13E248` in `assets/boot_elf.elf`,
  vram `0x0023D2C8`. Object-relative `.text+0x138` in `vobuf.o`.
- Semantics: ring-buffer emptiness test — returns `count == 0` for a `VoBuf`
  (`count` at offset `0xC`).
- Original instruction sequence:
  ```
  lw     $v0, 0xC($a0)    # load count
  jr     $ra
     sltiu $v0, $v0, 1    # delay slot: v0 = (count < 1)
  nop
  ```
- Ghidra decompile of `FUN_0023d2c8`: `return *(int*)(param_1 + 0xc) == 0;`
  (`sltiu x,1` on an unsigned/zero-based counter is the classic encoding of
  `x == 0`; the voBufReset note's layout inference said `count<1`, same thing).
- Sole caller: `jal 0x23d2c8` at vram `0x0023D2E4`, inside the sibling pop
  function at `0x23D2D8` (loads data/head/count/capacity, guards capacity with
  `beqzl -> break 7`, computes tag index). Direct absolute branch — no symbol
  resolution needed in the original.
- Symbol name is the bare `voBufIsEmpty` (no Itanium suffix), unlike siblings
  (`voBufIsFull__FP5VoBuf`, ...). Boot ELF carries no symtab, so the label comes
  from Splat; define it `extern "C"` with the bare name to keep future
  decompiled callers resolvable.

## C candidate

```cpp
extern "C" int voBufIsEmpty(VoBuf* self) {
    return self->count == 0;
}
```

No delay-slot trap here: only one memory op, nothing for the scheduler to
reorder. `volatile u32 count` (already in the struct from the voBufReset fix)
forces a single real load. EGC `-O2` emits `lw $v0,12(a0); jr $ra; sltiu
$v0,$v0,1`, identical to the original. (`self->count < 1` was not needed.)

## Verification (mechanical)

- Standalone EGC test compile first confirmed the exact words before touching
  the source.
- `make` builds; `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- `objdump -d build/code/game/movie/vobuf.o`: symbol `voBufIsEmpty` at
  `.text+0x138`, size `0xC`, words `8c82000c 03e00008 2c420001` — identical to
  the original slice at file offset `0x13E248`.
- `objdump -r build/code/game/movie/vobuf.o`: zero relocations, so no GP/data
  relocation risk in or around the new function.

## Next candidates in this file (same pattern)

- `voBufIsFull__FP5VoBuf` — expected `count == capacity` (xor + sltiu).
- `voBufDecCount__FP5VoBuf` — branch + dec, no delay-slot trap expected.
- `voBufGetData__FP5VoBuf` / `voBufGetTag__FP5VoBuf` — the 0x23D2D8 pop pair;
  data one has the `break 7` capacity guard (needs a matching C formulation).
