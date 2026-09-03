# func_001FF768 (code/game/hud.cpp)

- Original: 24 bytes at file offset `0x1006E8` in `assets/boot_elf.elf`:
  ```
  lw    $v0, -0x7308($gp)   # 8f828cf8
  beqz  $v0, .L             # 10400002
  addiu $v0, $v0, -1        # ffff4224  (beqz delay slot)
  sw    $v0, -0x7308($gp)   # af828cf8
  .L:
  jr    $ra                 # 0800e003
  nop                       # 00000000
  ```
- Splat vram address `0x001FF768`; object-relative `.text+0x930`
  (hud subsegment starts at file `0xFFDB8`).
- Semantics: `if (D_0015F8F8 != 0) D_0015F8F8--;` where `D_0015F8F8` is an
  int global at vram `0x15F8F8` (gp - 0x7308, gp = 0x166C00). Ghidra name
  `DAT_0015f8f8`; only xrefs are this function's own READ/WRITE. The address
  sits in the original ELF's loaded `lit` section (vram 0x15EF00-0x161228).

## Replacement

```cpp
extern "C" int D_0015F8F8;

extern "C" void func_001FF768(void) {
    if (D_0015F8F8 != 0) {
        D_0015F8F8--;
    }
}
```
plus `D_0015F8F8 = 0x15f8f8;` in `config/symbols.txt`, which makes splat emit
a `dlabel D_0015F8F8` inside `build/data/lit.lit4.s` at exactly that vram, so
the C reference links against the original data blob byte-for-byte.

## Key infrastructure findings (first C function using gp-relative globals)

- **Runtime `$gp`**: crt0 (`code/_generated/sce/crt0.s`) zeros all GPRs then
  `lui/addiu a0, %hi/%lo(D_00166C00); daddu $gp, $a0, $zero` — i.e. the game
  sets `$gp = address of D_00166C00 = 0x166C00` at boot (auto-labelled data
  symbol inside `build/data/data.data.s`). All gp-relative offsets baked into
  nonmatching asm blobs are relative to this value.
- **Link-time `_gp`**: the ps2 ld computes `_gp` itself (defaulted to a value
  that made our one `R_MIPS_GPREL16` fail with "relocation truncated to fit").
  Since `SCUS_971.99.ld` is splat-regenerated (gitignored), the fix lives in
  the Makefile link line: `--defsym _gp=0x166c00`. Any C code referencing a
  global within ±0x8000 of 0x166C00 now compiles to gp-relative access,
  matching the original. This is required for every future function that
  touches globals near core bss/lit/data (vram ~0x15EC80-0x1E8B78).
- **EGC `addu r,r,-1` quirk**: EGC 2.95.2 emits `addu $2,$2,-1` in its .s for
  an int decrement; GAS re-encodes that as `addiu v0,v0,-1` (0x2442ffff) in
  the object, so source `x--` matches original `addiu` encodings. Verified by
  objdump of a standalone test object before editing project files.
- `extern int D_...;` (no section attribute) is what makes EGC emit
  `R_MIPS_GPREL16`; do NOT add `__attribute__((section(".data")))` to an
  int global you need gp-relative for (that forces %hi/%lo, 2 instructions).

## Verification (mechanical)

- `nm -S build/code/game/hud.o`: `func_001FF768` at `.text+0x930`, size `0x18`.
- `readelf -r build/code/game/hud.o`: only relocations in `[0x930,0x948)` are
  two `R_MIPS_GPREL16 D_0015F8F8` at +0 and +0xC (resolve to offset -0x7308
  with `_gp=0x166c00`).
- Linked bytes at file `0x1006E8` in both ELFs:
  `f88c828f 02004010 ffff4224 f88c82af 0800e003 00000000` — identical.
- `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes (full ELF).
