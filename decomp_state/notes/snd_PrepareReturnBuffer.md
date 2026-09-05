# snd_PrepareReturnBuffer (vram 0x12DEF8, file 0x2EE78, 0x1C) — matched 2026-09-05

## Semantics

Prepares the IOP return buffer used by the 989snd command pipeline: records the
buffer pointer and index into two module globals, then zeroes slot 0 and the
sentinel slot `index + 1` of the buffer.

```c
void snd_PrepareReturnBuffer(int* buf, int index) {
    D_0015EC84 = index;
    D_0015EC80 = buf;
    buf[index + 1] = 0;
    *buf = 0;
}
```

`D_0015EC80` (pointer) / `D_0015EC84` (index) are the first two words of the
`.core_lit` section (0x15EC80/0x15EC84). The caller at 0x12E74C invokes it as
`snd_PrepareReturnBuffer(0x133100, 1)`; `snd_GotReturns` (0x12DE70) polls
`D_0015EC80` to detect IOP returns.

## Original body (7 words)

```
80100500  sll    v0, a1, 2
848085AF  sw     a1, 0x8084(gp)    # D_0015EC84
808084AF  sw     a0, 0x8080(gp)    # D_0015EC80
21104400  addu   v0, v0, a0
040040AC  sw     zero, 4(v0)
0800E003  jr     ra
000080AC  sw     zero, 0(a0)
```

## Codegen findings

- The two gp-relative stores come out in SOURCE statement order
  (`D_0015EC84 = index;` then `D_0015EC80 = buf;`) — same rule as the constant
  store tails, now confirmed for register stores of gp-window symbols.
- The independent `sll` for `buf[index + 1]` is hoisted to the top of the body,
  above the gp stores; the dependent `addu` lands right before its store.
  Natural statement order (`globals first, then the two zeroing stores`)
  reproduces the original scheduling with no tricks.
- Plain `extern void*` / `extern int` (no section attribute) for
  `.core_lit`-resident symbols emits direct `sw disp(gp)` with
  `R_MIPS_GPREL16` — no `lui`. 0x15EC80/84 sit 0x80/0x84 above the gp window
  bottom (gp 0x166C00 - 0x8000 = 0x15EC00), so GPREL16 just fits.
- PITFALL: Ghidra names these `puGpffff8080` / `iGpffff8084` (offset form).
  gp - 0x7F80 = 0x15EC80, NOT 0x15EE80 — the `D_0015EE8x` labels in `.lit`
  are different symbols 0x200 bytes away. Declaring the EE8x names linked
  cleanly but resolved 0x10000 apart and broke parity at exactly the two
  GPREL16 displacement bytes (0x8284/0x8280 instead of 0x8084/0x8080).
  Convert Ghidra Gp names to addresses (gp + signed offset) before naming.

## Verification (mechanical)

- `989snd.o` `snd_PrepareReturnBuffer`: 7 instructions, encodings identical to
  the original (sll/sw/sw/addu/sw/jr/sw).
- Relocs: `R_MIPS_GPREL16 D_0015EC84` @0x4d4, `R_MIPS_GPREL16 D_0015EC80`
  @0x4d8; final-ELF words 0x12DEF8..0x12DF13 byte-identical to the original.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- `decomp_status --count` 810 -> 809.
