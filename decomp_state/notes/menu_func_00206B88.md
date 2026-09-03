# func_00206B88 (code/game/menu.cpp)

- Original: 12 declared bytes at file offset `0x107B08` in
  `assets/boot_elf.elf`, vram `0x00206B88` (menu subsegment base vram
  `0x206978`; function at object-relative `.text+0x210`).
- Original instruction sequence (the final word is the `jr $ra` delay slot,
  owned by this function even though splat's declared size is 0xC):
  ```
  lui     $v1, 0x14            # 3c030014
  lbu     $v0, -11356($v1)     # 9062d3a4  (signed offset -0x2C5C)
  jr      $ra                  # 03e00008
  sltu    $v0, $zero, $v0      # 0002102b  (delay slot)
  ```
  i.e. `return D_0013D3A4 != 0;` for the unsigned byte at core.data vram
  `0x0013D3A4`.

## Address pitfall (important)

objdump annotates the load as `lbu v0,-11356(v1)`, which naively suggests
target `0x14D3A4` (hi|lo). MIPS lui/offset pairs use the SIGN-EXTENDED
immediate: `0x140000 - 0x2C5C = 0x13D3A4`. The true global is D_0013D3A4.
Cross-checks that confirmed this:

- spimdisasm's data-reference detection (run inside `make split`) had already
  registered `D_0013D3A4 = 0x13D3A4;` in `build/undefined_syms_auto.txt` from
  the nonmatching `func_00206B88.s`; nothing references `0x14D3A4`.
- Sibling getters resolve to consecutive bytes of the same block:
  | func       | signed offset | target   | dlabel in core.data.data.s |
  |------------|---------------|----------|----------------------------|
  | 0x206B88   | -11356        | 0x13D3A4 | (none; byte sits inside D_0013D39D's label range) |
  | 0x206B98   | -11355        | 0x13D3A5 | yes                        |
  | 0x206BA8   | -11354        | 0x13D3A6 | yes                        |
  | 0x206BB8   | -11353        | 0x13D3A7 | yes                        |
  | 0x206BC8   | -11347        | 0x13D3AD | yes                        |

Rule of thumb: for any `lui r,hi; lb/lw rt,off(r)` pair in these ELFs, the
target is `hi*0x10000 + sext16(off)`, never `hi*0x10000 + (off & 0xffff)`.

- Semantics: byte-flag predicate. Part of the menu byte-flag getter family
  documented in menu_func_002069A0.md / menu_func_002069B0.md. Ghidra has no
  function boundary here (region folded into FUN_00206978); the only xref is a
  DATA reference at core.data vram `0x19FF10` — an entry in a callback table.
  The table holds four consecutive entries `0x206B88..0x206BB8` (bytes
  D3A4..D3A7), plus `0x206B78` at `0x19FEF0` and `0x206BC8` at `0x19FF50`.
  Name kept as `func_00206B88`.
- The global has no dlabel in the generated data file (spimdis label
  placement quirk), but it is aliased in `build/undefined_syms_auto.txt`, so
  the extern from C binds correctly. No manual symbol-file edit was needed:
  re-running `make split` after the C edit kept the entry (splat derives it
  from references in the still-generated nonmatching .s files).

## C candidate

```cpp
extern u8 D_0013D3A4 __attribute__((section(".data")));

extern "C" int func_00206B88(void) {
    return D_0013D3A4 != 0;
}
```

No delay-slot trap: identical shape to the matched siblings. EGC `-O2` emits
`lui $v1, %hi(D_0013D3A4); lbu $v0, %lo(...); jr $ra; sltu $v0,$zero,$v0`,
identical to the original.

## Verification (mechanical)

- `nm build/code/game/menu.o`: `func_00206B88` at `.text+0x210`, plain `T`,
  no `.NON_MATCHING` alias; neighbours func_00206B78 at `.text+0x200` and
  func_00206B98 at `.text+0x220` — contiguous exactly as the original.
- `build/boot_elf.elf` slice at file offset `0x107B08..0x107B18`:
  `1400033ca4d362900800e0032b100200` == original slice (all 4 words incl.
  delay slot).
- `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.

## Next candidates in this file (same pattern)

- func_00206B98 / BA8 / BB8 / BC8 — D_0013D3A5/A6/A7/AD, all dlabelled and
  already referenced; should be trivial one-liners like this one.
- func_00206B78 is NOT the same pattern: it does `lw $v0, -0x6E9C($gp); jr
  $ra; sltiu $v0,$v0,1` — a gp-relative load from `.lit` (D_0015FD64 =
  `.float 0`), i.e. it always returns 1. Needs a separate investigation of
  how the original source produced a literal-pool integer load here.
