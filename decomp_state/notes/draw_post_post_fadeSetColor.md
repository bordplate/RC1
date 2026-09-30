# fadeSetColor (0x1F5210, 0x90) — MATCHED 2026-09-30

Fade-color setter (C linkage, unmangled `func_001F5210` → `fadeSetColor`).
Loads the fade GS register (`VU1_FADE_GS_REG` = 1) with a packed color and
re-streams the fade GS state block:

```cpp
void fadeSetColor(int r, int g, int b, int intensity) {
    VU1_addGSregister(VU1_FADE_GS_REG, (unsigned long)r | ((unsigned long)g << 8)
                      | ((unsigned long)b << 0x10) | ((unsigned long)intensity << 0x18));
    vu1ChainHead[0] = VU1_DATA_REF_TAG | 0x14;
    vu1ChainHead[1] = (u32)gsStateFadeColor;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 0x14;
    vu1ChainHead = vu1ChainHead + 4;
}
```

`r` is the low byte, `intensity` the top byte of the register. Callers:
`FadeToBlack` (`fadeSetColor(0,0,0,VU1_FADE_FULL)`) and `DrawDebugProfiler`
(three calls: `(0,0,0,v)`, `(0xff,0xff,0xff,v)`, and a 4-global colored form).

## Codegen findings (default flags, draw_post_post.o)

1. 64-bit packing needs a per-term cast. The original packs with `dsll`/`or`
   (64-bit): `dsll a1,a1,8; dsll a2,a2,16; or a1,a0,a1; or a1,a1,a2;
   dsll a3,a3,24; or a1,a1,a3`. With `int` args and a single trailing
   `(unsigned long)` cast the shifts come out 32-bit `sll` plus a zero-extend
   (wrong). Casting EACH shifted term — `(unsigned long)g << 8` etc. — makes
   EGC emit the 64-bit `dsll`/`or` with no extra extend, identical to the
   matched `FadeToBlack` per-frame write `(unsigned long)(expr) << 24`.
   The first term (`r`) is unshifted; `(unsigned long)r` elides to a plain
   `or a1,a0,a1`.

2. Final head store is ABSOLUTE, standalone before `jr`:
   `addiu v0,v0,16; lui at,%hi; sw v0,%lo(at); jr ra; addiu sp,sp,16`.
   This is the double-volatile form (`vu1ChainHead = vu1ChainHead + 4`),
   NOT the plain `vu1ChainHeadStore` alias, which instead emits the GPREL
   `sw r,-0x5D00(gp)` in the `jr` delay slot (the `VU1_gsRegsNormal` form,
   0x233BC8). The packet stores ([0]-[3]) are the same five self-based
   `lui/lw` head reloads the matched `FadeToBlack`/`ResetGsRegisters` use.

## Data

`gsStateFadeColor` (0x13CC90) is an 80-word GS state block (tag 0x14 = 20×4
words). It is a 0x40-offset sibling of `gsStateFade` (0x13CDD0): the two
share their 0x40-byte GS-state lead-in and most of their body, so the blocks
overlap. Named in config/symbols.txt.

## Verification

Standalone object objdump instruction-for-instruction identical to the
original; `tools/tu_assembler_diff.py` 71/71 match for draw_post_post.o
(includes caller FadeToBlack); `make` + `cmp build/boot_elf.elf
assets/boot_elf.elf` byte-identical.
