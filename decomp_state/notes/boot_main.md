# main (vram 0x12D9D8, file 0x2E958, 0x4C bytes) — MATCHED 2026-09-05

The game's main loop. Replaced `INCLUDE_ASM(..., main)` in code/game/boot.cpp:84:

```cpp
int main() {
    StartLevelPtr f = startlevel;
    while (true) {
        f();
        f = ParseBin();
        FlushCache(0);
        FlushCache(2);
    }
}
```

(`StartLevelPtr`/`startlevel`/`ParseBin` prototypes already existed in boot.cpp;
`FlushCache(int)` in include/common.h.)

## Semantics (Ghidra agrees)

Run `__main` (C++ static-init trampoline, 0x11DCC8 in sce/lib.s), then loop
forever: call the current level function pointer (initially `startlevel__Fv`,
0x1E9658, the boot/main-menu level), re-point it at `ParseBin__Fv`'s return
value (level loader that copies the next level's chunks and returns its entry),
then `FlushCache(0); FlushCache(2);` (cache ops around the swap).

## Codegen findings (new EGC behaviors)

- **EGC auto-emits `jal __main` at the top of a C++ `main`** — even in a TU
  with no global constructors (boot.cpp has none). Writing an explicit
  `__main();` in the source emits a SECOND `jal __main` (21 words instead of
  19). The original single `jal __main; nop` is compiler-inserted; the source
  must NOT call it. No `extern` declaration needed either.
- `f = ParseBin()` (ParseBin is INCLUDE_ASM'd earlier in the same file, i.e. at
  .text offset 0 of this object): EGC/gas emits the `jal` with an R_MIPS_26
  relocation against the `.text` SECTION symbol (addend 0) rather than the
  `ParseBin__Fv` FUNC symbol. Links identically because ParseBin sits at the
  section origin (final .text base == ParseBin address). Don't treat this
  relocation as broken — verify by link, not by symbol name.
- The `lui %hi; addiu %lo; nop` for `f = startlevel` is emitted verbatim
  (the trailing nop is real, not a delay slot); the loop back-edge is
  `b; nop` with the whole body scheduled as
  `jalr; nop / jal ParseBin; nop / daddu a0,0,0 / jal FC / <ds daddu s0,v0,0> /
  jal FC / <ds addiu a0,0,2> / b; nop` — the `f = ParseBin()` store is pulled
  into the first FlushCache delay slot, matching the original exactly with
  natural statement order (no reordering tricks needed).

## Verification

- Object: 19/19 words of build/code/game/boot.o `main` (0xe0..0x134) equal the
  original ELF words at 0x12D9D8..0x12DA24 (compared via objdump -d on both;
  jal targets start as 0x0c000000 + R_MIPS_26 relocs).
- Reloc targets: R_MIPS_26 __main, HI16/LO16 startlevel__Fv (gives original
  `lui 0x1f`/`addiu 0x9658` — %hi rounds up because %lo 0x9658 >= 0x8000),
  R_MIPS_26 .text (=ParseBin, section origin), R_MIPS_26 FlushCache x2.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes (PARITY OK).
- decomp_status count 816 -> 815.
