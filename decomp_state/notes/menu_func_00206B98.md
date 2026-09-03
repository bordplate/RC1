# func_00206B98 (code/game/menu.cpp)

- Original: 16 bytes at file offset `0x107B18`, vram `0x00206B98`.
- Instruction sequence:
  ```
  lui     $v1, 0x14            # 3c030014
  lbu     $v0, -11355($v1)     # 9062d3a5  (signed offset -0x2C5B)
  jr      $ra                  # 03e00008
  sltu    $v0, $zero, $v0      # 0002102b  (delay slot)
  ```
  i.e. `return D_0013D3A5 != 0;` — target is `0x140000 - 0x2C5B = 0x13D3A5`
  (lui/offset pairs use the sign-extended immediate; see
  menu_func_00206B88.md for the pitfall).

## C candidate

```cpp
extern u8 D_0013D3A5 __attribute__((section(".data")));

extern "C" int func_00206B98(void) {
    return D_0013D3A5 != 0;
}
```

Identical shape to matched siblings func_002069A0 / 002069B0 / 00206B88 —
same menu byte-flag getter family, entries in the callback table at
core.data vram `0x19FF14` (consecutive after 0x206B88's entry at 0x19FF10).

`D_0013D3A5` has no dlabel in the generated core.data file; it is aliased in
`build/undefined_syms_auto.txt` (line 66) and survives `make split` after the
INCLUDE_ASM removal — spimdis re-registers the alias from the C extern
reference.

## Verification (mechanical)

- `nm build/code/game/menu.o`: `func_00206B98` at `.text+0x220`, plain `T`,
  no `.NON_MATCHING` alias; neighbours func_00206B78 at `.text+0x200` (still
  nonmatching) and func_00206BA8 at `.text+0x230` — contiguous as original.
- `build/boot_elf.elf` slice at file offset `0x107B18..0x107B28`:
  `1400033ca5d362900800e0032b100200` == original slice (all 4 words incl.
  delay slot).
- `make split && make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf`
  passes; decomp_status count 865 -> 864.
