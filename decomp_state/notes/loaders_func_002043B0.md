# loaders func_002043B0 (0x2043B0, 0x78) — level archive placement

## Purpose
Places the decompressed level archive and sets up its stream. `levelRoot`
(0x15EE4C) is pointed just below the runtime base `levelLoadPlacementTop`
(0x1FF7FF0), offset by the level's sector count, rounded down to a 0x10
boundary. The first level word is set to 0x60 and `func_00216728` (stream
setup, still INCLUDE_ASM in stream.cpp) is called with
`(levelRoot + *levelRoot, bootAssets.dest, bootAssets.sectors)`. Returns 1.

## Key decode
```c
x = (bootAssets.sectors << 11 + 0x1057) & 0xFFFFF000   // sector*0x800 + 0x1057, page-aligned
levelRoot = (0x1FF7FF0 - x) & -0x10                    // -0x10 is a provable no-op
*levelRoot = 0x60
func_00216728(levelRoot + *levelRoot, bootAssets.dest, bootAssets.sectors)
```

`bootAssets` (0x137B80) tail record: `s32 dest` at +0x4F8, `s32 sectors` at
+0x4FC (added to BootAssetTable in boot_level.h; splits the former pad with
identical layout).

## Why the literal 0x1FF7FF0 does NOT match (the wall)
Using the numeric literal `0x1FF7FF0` left a coupled 20-diff EGC wall:
- EGC materializes an integer literal's low part with `ori`, but the original
  uses a signed `addiu %lo` (an ADDRESS relocation does `%hi/%lo` -> `lui` +
  signed `addiu`).
- EGC folds the no-op `& -0x10` entirely (it can prove both operands are
  0x10-aligned); the original keeps it.
- The `0x1FF7FF0`/`0xFFFFF000` constants landed in swapped registers and the
  `-0x10` mask materialized as a 2-instruction `li/ori` (0xFFFFFFF0) instead of
  a single early `addiu -0x10`.

## The fix (verified 0-diff by last-resort, matched in full build)
`0x1FF7FF0` is a runtime base, not an ELF symbol. Declare it as a byte-array
ADDRESS via a linker alias (`config/linker_aliases.ld`):
```ld
levelLoadPlacementTop = 0x01FF7FF0;
```
```c
extern u8 levelLoadPlacementTop[];
levelRoot = (LevelRoot*)((s32)(
    (u32)levelLoadPlacementTop -
    (((bootAssets.sectors << 11) + 0x1057) & 0xFFFFF000)
) & -0x10);
```
Because it is an address, the subtraction becomes a genuine `%hi/%lo`
relocation (signed `addiu` lo) and EGC cannot prove the alignment of an
unresolved `u8[]` base, so the `& -0x10` is not folded. The single expression
also reproduces the original constant register allocation and the early
`addiu -0x10` / `addiu 0x60` scheduling. The arg0 reloads `*levelRoot` via a
plain `*(u32*)levelRoot` deref (not a CSE'd constant). Callee declared
`void func_00216728(void*, s32, s32)` (matches the original caller codegen;
the callee actually returns 1 but the declared prototype drives the caller's
register allocation). See the Lombyte control
`reference/Lombyte/src/textbin/fun_002043b0.c`.

Operator-precedence trap: the `& 0xFFFFF000` must bind to the inner
`(sectors << 11) + 0x1057` term (extra parens), not to the whole subtraction.

## Result
Probe 0-diff (120 B). Clean make clean+split+make, cmp byte-identical,
count 528 -> 527.
