# setMessageText (func_001FF658) + dead tail func_001FF5E8

The `INCLUDE_ASM` for `func_001FF5E8` in code/game/hud_post_post2.cpp spanned
0xF0 bytes (0x1FF5E8-0x1FF6D7) and contained two parts:

1. A 0x70-byte **dead `addiu $sp` tail** at 0x1FF5E8-0x1FF657 (twelve units
   0x60,0x20,0x60,0x40,0x40,0x90,0x10,0x90,0x10,0x20,0x10,0x130 with
   interleaved nops). `tools/deadness_scan.py 0x1FF5E8`: 0 references; no
   Ghidra function. Not a function — byte-preservation assembly.
2. The real function **setMessageText** at 0x1FF658-0x1FF6D7 (0x80 bytes),
   originally the auxiliary symbol `func_001FF658` (an `alabel` inside the
   `func_001FF5E8` glabel).

## Real function (setMessageText)

```c
void setMessageText(char* str);
void setMessageText(char* str) {
    if (strlen(str) < 0x50) {
        memcpy((void*)messageTextBuffer, (void*)messageTooLongLabel, 19);
    }
    func_001165B8((u8*)messageTextBuffer, (u8*)str);
}
```

- `str` arrives in a0 and is saved to s0 (restored at the epilogue).
- `strlen(str) < 0x50` -> `sltiu v0,v0,0x50; beqz v0,skip`.
- When true, a 19-byte copy of `messageTooLongLabel` ("(Message too long)\0",
  18 chars + null) into `messageTextBuffer`. EGC inlines the 19-byte
  `memcpy` as two unaligned 64-bit moves (`ldl/ldr` x2 -> `sdl/sdr` x2) plus a
  3-byte `lb`/`sb` tail. The aligned `ld`/`sd` form (from a `u64*` cast) is 4
  words shorter and does NOT match — the pointer type must stay `u8*`/`char*`
  so EGC emits the unaligned `ldl/ldr`.
- `func_001165B8(dst, src)` is a SIMD null-terminated string copy (a0=dst,
  a1=src; `psubb`/`pand`/`pcpyld` to find the null run). Called with
  (messageTextBuffer, str).
- The `lui a0,%hi(messageTextBuffer)` for the call argument is hoisted into the
  `beqz` delay slot and reused as the `addiu a0` in the `jal` delay slot.

## Symbols

New data symbols added to config/symbols.txt (both in .data):
- `messageTextBuffer = 0x0019A440;`  (all-zero message text buffer)
- `messageTooLongLabel = 0x001E7AA8;` ("(Message too long)" string)
`strlen` (0x001166CC) was already a symbol. `func_001165B8` is a resident
core.text SDK helper (declared `extern "C"` in the file).

## Symbol / layout handling

- The dead tail is emitted as a file-scope `asm(...)` block at the top of the
  TU (same source position as the removed INCLUDE_ASM), preserving the
  `nonmatching func_001FF5E8, 0x70` / `glabel` / `endlabel` structure so the
  linker-script pin `func_001FF5E8 @ 0x1ff5e8` stays defined. Precedent:
  code/game/actuator.cpp dead tails.
- The C function uses its NATURAL C++ symbol `setMessageText__FPc`, NOT an
  `asm("func_001FF658")` override. func_001FF658 is an auxiliary (alabel)
  symbol in the original ELF, so Splat keeps it in the linker script's
  ABSOLUTE block (`func_001FF658 = 0x1ff658`). A global C symbol with the same
  name would be a multiple-definition conflict. The absolute symbol already
  points at 0x1ff658 (where the 0x80-byte C function lands after the 0x70-byte
  dead tail), so the four `jal func_001FF658` callers (func_00215130 x2,
  func_002151D8, func_00216C48) still resolve correctly.

## Verification

- Probe: `decomp_probe.py` on the real function -> match, 0/128 word diffs.
- Full: `make split && make -j2 && cmp build/boot_elf.elf assets/boot_elf.elf`
  byte-identical.
- Built region 0x1FF658 disassembles identically to the original (addiu
  sp,-0x20; sq s0; sq ra; jal strlen; sltiu v0,0x50; beqz; ...).
- Nonmatching count 547 -> 546.
