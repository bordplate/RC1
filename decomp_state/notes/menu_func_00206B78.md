# func_00206B78 (code/game/menu.cpp) — MATCHED

Menu callback-table getter. vram `0x00206B78`, file offset `0x107AF8`, size 0xC.

## Original body

```
lw      $v0, -0x6E9C($gp)   # 6491828F (splat word display is byte-swapped)
jr      $ra
sltiu   $v0, $v0, 0x1       # delay slot: UNSIGNED compare vs 1
```

Semantics: `return D_0015FD64 < 1;` for an unsigned int at vram `0x0015FD64`.
With `_gp = 0x166C00` (Makefile `--defsym _gp=0x166c00`, crt0 loads D_00166C00),
`0x166C00 - 0x6E9C = 0x15FD64`.

## Globals and table context

- `D_0015FD64` is a word inside the `lit` segment (vram 0x15EF00+), defined in
  `code/_generated/build/data/lit.lit4.s` as `dlabel D_0015FD64` (`.float 0`,
  i.e. zeroed int field between the strings "l 18\0..." at 0x15FD54 and
  "map.cpp\0" at 0x15FD68). No C definition needed; a plain
  `extern "C" unsigned int D_0015FD64;` binds to the lit-segment label.
- The function is entry [0] of a function-pointer table at vram `0x19FEF0`
  (`.data`, file offset 0xA0E70): `[0]=0x206B78, +0x20: 0x206B88/98/A8/B8,
  +0x60: 0x206BC8, +0x80: 0x206BD8, 0x206E18, ...` — see
  menu_func_00206B88.md for the sibling byte-flag getters. No jal/j callers in
  core.text; only DATA (table) references. Ghidra has no function boundary or
  xrefs at 0x206B78 (analysis gap in this region).

## Codegen notes

- EGC 2.95.2 (`-G8 -O2`) emits `lw $v0, off($gp)` for a plain extern
  (R_MIPS_GPREL34 filled by the linker), same recipe as the matched
  `D_0015F49C` / `D_0015F8F8` globals in draw.cpp / hud.cpp.
- Signedness matters: with `int`, EGC emits `slti v0,v0,1`; with `unsigned
  int` it emits `sltiu v0,v0,1` matching the original. EGC does NOT fold
  unsigned `< 1` into an equality test (unlike its `!= 0` -> `sltu $v0,$0,$v0`
  fold seen in the byte getters), so the compare-vs-1 shape is preserved.

## Candidate

```cpp
extern "C" unsigned int D_0015FD64;

extern "C" int func_00206B78(void) {
    return D_0015FD64 < 1;
}
```

## Verification (2026-09-04)

- `objdump -d build/code/game/menu.o`: `lw v0,0(gp); jr ra; sltiu v0,v0,1`
  at func_00206B78; `nm` shows `U D_0015FD64`.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes (byte-for-byte,
  which mechanically covers the function's linked GP-relative offset -0x6E9C).
- `decomp_status.py --count`: 860 -> 859.

Status: committed. Name kept as func_00206B78 (field semantics at 0x15FD64
unknown; likely a non-negative counter/state int where `< 1` means "empty/zero").
