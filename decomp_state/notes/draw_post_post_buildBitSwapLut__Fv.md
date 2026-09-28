# buildBitSwapLut__Fv (0x001F7A30, 84 bytes)

256-entry lookup-table init. Builds `bitSwapLut` (0x18E740, `u32[256]`).

## What it does

```c
for (int i = 0; i < 256; i++) {
    int index = i & 0xE7;        // clear bits 3 and 4
    if (i & 0x8)  index |= 0x10; // if bit 3 set -> set bit 4
    if (i & 0x10) index |= 0x8;  // if bit 4 set -> set bit 3
    bitSwapLut[index] = (u32)((i >> 1) << 24);  // top byte = i>>1
}
```

So `index` is `i` with bits 3 and 4 swapped (a bijection over 0..255), and the
entry's top byte is `i >> 1`. The loop is 256 iterations; the `sw` sits in the
`bnez` taken-delay slot.

Single caller: `InitOnce__Fv` at 0x201984 (`jal buildBitSwapLut__Fv`).

## Match

Matched byte-for-byte on the first C attempt (0 word diffs of 21). No flags,
no barriers. The natural C produces the exact bit-op sequence and register
allocation:

| step | original reg | EGC reg |
|------|-------------|---------|
| base (0x18E740) | t0 ($8), hi temp v0 ($2) | t0, hi temp v0 |
| loop index i | a3 ($7) | a3 |
| i & 0xE7 | v0 ($2) | v0 |
| i & 0x8 | a1 ($5) | a1 |
| \|0x10 temp | v1 ($3) | v1 |
| i & 0x10 | a2 ($6) | a2 |
| i>>1 / <<24 | a0 ($4) | a0 |

Instruction order: `lui v0,hi; move a3,zero; addiu t0,v0,lo; nop` then the
loop body `andi/ori/andi/movn/sra/ori/sll/movn/addiu/sll/slti/addu/bnez` with
the `sw` in the delay slot, then `jr ra; nop`.

## Match-critical detail

The table must be a **named `.data` symbol** (`bitSwapLut`,
`__attribute__((section(".data")))`) so EGC emits the self-based base load
(`lui v0,%hi; addiu t0,v0,%lo`). A raw constant address `(u32*)0x18E740`
instead materializes the constant as `lui t0,hi; ori t0,t0,lo` (hi and lo both
in one register), which breaks the match. The 0x18E740 address is out of the
gp window, so a plain `extern` without the `.data` section attribute would
emit a GPREL access and fail to link.

## Renames

- `func_001F7A30` -> `buildBitSwapLut__Fv` (config/symbols.txt).
- `D_0018E740` -> `bitSwapLut` (config/symbols.txt).
- Caller `.s` (InitOnce__Fv) regenerated to `jal buildBitSwapLut__Fv`.
