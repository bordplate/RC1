# func_00233038 (vram 0x233038, file 0x133FB8, 0x28) — matched 2026-09-05

## Semantics

Bounds-checked lookup into `D_001DD1D8`, a 64-entry array of 16-byte stash
streaming slots (4 ints each). Returns the second word (`f1`) of slot `i`
when `i < 0x40`, else the error code `-3`. `D_001DD1D8` (vram 0x1DD1D8,
0x400 bytes) is the same table the sibling stash functions
`func_00232D00` / `Stash_SendData` / `func_00232F20` index with the same
`lui %hi / sll 4 / addiu %lo / addu` base arithmetic, so the entry is a
16-byte struct and the returned field is the second int.

## Original body (file 0x133FB8, 10 words)

```
2c820040  sltiu  v0, a0, 0x40
10400006  beqz   v0, .L00233058
3c030016   lui   v1, %hi(D_001DD1D8)
00041100  sll    v0, a0, 4
2463d1d8  addiu  v1, v1, %lo(D_001DD1D8)
00621821  addu   v1, v1, v0
03e00008  jr     ra
8c620004   lw    v0, 4(v1)
.L00233058:
03e00008  jr     ra
2402fffd   addiu v0, 0, -3
```

## Replacement (code/game/stash.cpp)

```cpp
typedef struct {
    int f0;
    int f1;
    int f2;
    int f3;
} StashEntry16;

extern "C" StashEntry16 D_001DD1D8[64];

extern "C" int func_00233038(unsigned param_1) {
    if (param_1 >= 0x40) {
        return -3;
    }
    return D_001DD1D8[param_1].f1;
}

asm("nop");
asm("nop");
```

`unsigned` param is required: the original compares with `sltiu` (unsigned).
A signed param makes EGC emit `slti`. `D_001DD1D8[64]` of a 16-byte struct
reproduces the `sll 4` index scale (64*16 = 0x400, matching the symbol size).

## Codegen finding — trailing section padding

The original `stash.o` `.text` region is **0x388** bytes (file subsegment
0x133C60..0x133FE8): the five stash functions total 0x380, followed by **8
bytes of trailing nops** (2 words at vram 0x233060/0x233064) after
`func_00233038`, the file's last symbol. The C function emits only its 0x28
body, so `stash.o` came out 0x380 — 8 bytes short. Because the linker packs
`.text` object sections back-to-back, that shifted `tfragfunc.o` (and every
later section) 8 bytes early and broke full-ELF parity (first diff at file
0xEA540, 33411 bytes total).

The 2 nops are compiler section-tail padding, not part of the function's
logic (the symbol is 0x28 / 10 words; `SetTfragDists__Fv` in the next object
starts clean at 0x233068 with no leading nops). They are supplied with two
file-scope `asm("nop");` after the function — same idiom as `ParseBin` in
`code/game/boot.cpp` (11 `asm("nop")` to reproduce original padding).

## Verification (mechanical)

- `stash.o` `func_00233038` body: 10 instructions, encodings identical to
  the original (sltiu/beqz/lui/sll/addiu/addu/jr/lw/jr/li).
- Relocs: `R_MIPS_HI16 D_001DD1D8` @0x360 + `R_MIPS_LO16 D_001DD1D8` @0x368
  resolve to %hi=0x16 / %lo=0xd1d8, matching the original constant pair.
- `stash.o` `.text` size = 0x388 (904 bytes) == original stash subsegment
  span, restoring `tfragfunc.o` to vram 0x233068.
- Built slice file 0x133FB8..0x133FE7 (48 bytes incl. the 2 tail nops) ==
  original.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- `decomp_status --count` 812 -> 811.
