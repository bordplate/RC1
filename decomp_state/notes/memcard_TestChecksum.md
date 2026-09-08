# memcard_TestChecksum (0x20AD38, 60 bytes) — MATCHED 2026-09-08

Save-record CRC verification wrapper in code/game/memcard.cpp. `data` points at
a record header `{ int len; int crc; payload... }`; returns 1 iff
`data[1] != 0 && memcard_Checksum(data + 2, data[0]) == data[1]`, else 0.
Callers: 0x20AE7C (restore path, result compared to 0) and 0x20AF54
(`bnezl v0` on the result).

```c
extern "C" int memcard_Checksum(int* data, int len);

extern "C" int memcard_TestChecksum(int* data) {
    int stored_crc = data[1];
    int result = 0;
    int len = data[0];
    if (stored_crc != 0)
        result = memcard_Checksum(data + 2, len) == stored_crc;
    return result;
}
```

## Codegen findings

- The crux was the SCHEDULING of two independent instructions around the
  `beqz`. Original:
  ```
  addiu sp,sp,-32
  move  v0,zero      ; b=0, ABOVE the prologue saves
  sq    ra,16(sp)
  sq    s0,0(sp)
  lw    s0,4(a0)     ; stored_crc
  beqz  s0,end
    lw   a1,0(a0)    ; len load IN the beqz delay slot
  jal   memcard_Checksum
    addiu a0,a0,8    ; data+2 in the jal delay slot
  xor   v0,v0,s0
  sltiu v0,v0,1
  ```
- With `int len` declared INSIDE the if (or `data[0]` passed directly), EGC
  puts the `move v0,zero` in the beqz delay slot and the len `lw` in the
  mainline — a stable 5-word diff across ~40 tested forms (declaration order,
  unsigned, struct field forms, const, temp pointers, register asm, comparison
  rewrites, ternary, direct-return layouts, do-while, C vs C++, and every
  scheduler-flag combo: -fno-schedule-insns / -fno-schedule-insns2).
- Hoisting `int len = data[0];` BEFORE the if (statement order: crc load,
  b=0, len load, if) flips the scheduler: len's load is now live before the
  branch and takes the beqz delay slot, and the zero hoists above the sq
  saves. The delay slot executes on both paths, so the pre-branch load is
  consistent with the source. Found by last-resort-decompiler (GPT-5.6 Sol)
  after the primary agent exhausted the form space.
- Allocation (unchanged across all forms): pointer stays in a0 (argument
  register), `stored_crc` in s0 (saved across the call), b/crc in v0.
  `==` on ints compiles to `xor v0,v0,s0; sltiu v0,v0,1`.
- Default flags; no PRIVATE_COMPILE_FLAGS needed.

## Splat misdecode quirk (verified 2026-09-08)

The generated .s line for 0x20AD3C reads
`/* 10BCBC 0020AD3C 2D100000 */ daddu $2, $0, $0` but the instruction is
`move v0,zero` (word 0x0000102D): the third comment field is the raw on-disk
bytes (LE word reversed, per the documented Splat quirk), and Splat's own
disassembler misdecodes 0x0000102D (or v0,zero,$0) as `daddu $2,$0,$0`.
objdump of assets/boot_elf.elf is ground truth. Corollary: the 313
"daddu $2,$0,$0" occurrences in the boot ELF are all `move v0,zero` — EGC's
`return 0` / `int x = 0` idiom is the or-mv encoding, and prior notes that
refer to a hoisted "daddu v0,0,0" mean this move.

## Verification

- Probe: decomp_state/probes/memcard_TestChecksum_v13.cpp —
  decomp_probe.py match:true, 0 differences (60/60 bytes).
- decomp-verifier: object bytes identical (modulo the R_MIPS_26 jal reloc),
  full memcard.o text region 0x209030-0x20B3BF unchanged, `cmp
  build/boot_elf.elf assets/boot_elf.elf` byte-for-byte PASS.
- Nonmatching count 741 -> 740.
