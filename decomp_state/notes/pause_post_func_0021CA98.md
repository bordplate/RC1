# func_0021CA98 (vram 0x21CA98, file 0x11DA18, 44 bytes) — MATCHED 2026-09-11

Pause-menu callback: pick one of two sprite-index lists from a dedicated flag
and store the chosen list pointer in the menu item's field at offset 0x34.

```cpp
int pause_selectSpriteList(PauseSpriteListMode* mode) {
    mode->spriteList =
        (pauseSpriteListFlag != 0) ? (u32)pauseSpriteListA : (u32)pauseSpriteListB;
    return 0;
}
```

## Identity / context

Direct sibling of the already-matched `SetPauseActionList` (0x21A1B0): both
read a flag, pick one of two list pointers, store it in the menu item's field
at offset 0x34, and return 0. This one is the third callback in the data table
at 0x1D07F8 (`func_00219D80`, `func_0021A328`, `func_0021CA98`, then 0x90 / 4),
and the generated data references it by name at 0x1D0800 (`.word func_0021CA98`).

The object (`mode`) is a pause-menu item whose 7-entry pointer array begins at
offset 0x30. `func_0021CA60` (immediately before) copies those 7 u32 to the
global `D_00141EA0`; `func_0021C9C8` copies them back and counts the
non-zero entries. This callback sets element 1 (offset 0x34) to a pointer to
one of two flat index lists, so after the copy `D_00141EA0[1]` is that pointer.

- `pauseSpriteListFlag` (D_0013D4C2, u8): the select flag. Out of the gp
  window, so declared `__attribute__((section(".data")))` to force the absolute
  `lui/lbu`. It is referenced by NO other function.
- `pauseSpriteListA` (D_001D06D0) / `pauseSpriteListB` (D_001D0708): two flat
  `u32[14]` lists (13 x 16-bit indices 0x4F0D.. plus a null terminator). The
  first ten entries are identical; only the last three differ
  (A: 0x4F17,0x4F19,0x4F18 ; B: 0x4F1A,0x4F1C,0x4F1B).

## Codegen findings

The original branches `beql v0,$0` to the flag==0 path:

```
lui    $3, %hi(D_0013D4C2)
lbu    $2, %lo(D_0013D4C2)($3)
beql   $2, $0, .L            ; flag==0 -> .L
    lui  $2, %hi(D_001D0708)  (delay)
lui    $2, %hi(D_001D06D0)
b      .L2
    addiu $2, $2, %lo(D_001D06D0) (delay)
.L:   addiu $2, $2, %lo(D_001D0708)
.L2:  sw    $2, 0x34($4)
jr     $ra
    daddu $2, $0, $0           (misdecoded move v0,zero; returns 0)
```

Semantics: `flag != 0 -> D_001D06D0 (A)`, `flag == 0 -> D_001D0708 (B)`.

EGC's ternary branch direction depends on the comparison sense: the
`(flag != 0) ? A : B` form emits `beqz` to the `: B` path (matching the
original layout). The `(flag == 0) ? B : A` form emits `bnez` to the B path,
giving a 3-byte diff (branch opcode + the two swapped `addiu` slots). So the
`!= 0` ternary is the matching form here even though `SetPauseActionList` uses
an `if/else`.

## Source

Replaced the INCLUDE_ASM in code/game/pause_post.cpp (was line 81) with the
struct, externs, and function shown above. C++ linkage; the mangled symbol
`pause_selectSpriteList__FP19PauseSpriteListMode` lands at 0x21CA98 purely by
contiguous `.text` object layout (the function sits exactly where the
INCLUDE_ASM was, between func_0021CA60 and the pause_resetMemoryCardState
block). Added to config/linker_aliases.ld:

```
func_0021CA98      = 0x0021CA98;   ; name referenced by generated data @0x1D0800
pauseSpriteListFlag = 0x0013D4C2;
pauseSpriteListA    = 0x001D06D0;
pauseSpriteListB    = 0x001D0708;
```

## Verification

- Built function words (0x21CA98..0x21CAC4, 44 bytes) byte-identical to the
  original:
  `1400033c c2d46290 04004050 1d00023c 1d00023c 02000010 d0064224 08074224 340082ac 0800e003 2d100000`
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes
  (md5 `0c081d4e0353b419d95e5f95eb0ebacb` for both).
- decomp_status --count: 722 -> 721.
