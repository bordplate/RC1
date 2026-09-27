# GetOcclGridFromPair__Fiiiiiif (0x1F2768, 184 bytes) — MATCHED

`void GetOcclGridFromPair(int cell0X, int cell0Y, int cell0Z, int cell1X,
int cell1Y, int cell1Z, float fraction)` in `code/game/draw_post_post.cpp`.

## Semantics

Fetches an occlusion-grid cell from a pair of adjacent octree cells, trying the
nearer one first. Each cell is a three-level (z, y, x) index resolved by the
blocked sibling `ParseOcclGrid` (0x1F2690), which returns the grid-cell pointer
or 0. The float `fraction` is the caller-computed fractional part of the scaled
cell coordinate: the caller (BuildOcclVisibility, 0x1F2820) passes
`(pos - (float)(int)pos)` together with the cells `(i-1, j, k)` and `(i+1, j, k)`
where `i = (int)pos`. Below 0.5 the first cell is nearer and is parsed first;
at/above 0.5 the second is. If the first returns non-zero (a cell was found) the
function stops; otherwise it parses the other cell (return value ignored).

```cpp
if (fraction < 0.5f) {
    if (ParseOcclGrid(cell0...) != 0) return;
    second = cell1;
} else {
    if (ParseOcclGrid(cell1...) != 0) return;
    second = cell0;
}
ParseOcclGrid(second...);
```

## Match-critical shape: the shared second jal

The original emits ONE `jal ParseOcclGrid` for the second (fallback) call at
0x1F27F4; both branches converge on it. Path A (fraction < 0.5) sets
`a0/a1/a2 = (s0,s1,s2)` then `b 0x1F27F4`; path B (fraction >= 0.5) sets
`a0/a1/a2 = (s3,s4,s5)` then falls through. The C source therefore must set the
second cell's coords into locals in each branch and make a SINGLE trailing call.
A naive per-branch second call (`if (...) ParseOcclGrid(a); else
ParseOcclGrid(b);`) makes EGC emit two separate `jal`s and overshoots by 4 bytes
(188 vs 184) — this was the only diff in the first candidate.

## ABI / scheduling

- The float arg arrives in `$f12` (first float register of the EE ABI); callers
  do `lwc1 $f12; mul.s $f12,$f12,$f20`. The 0.5f constant loads `lui at,0x3f00;
  mtc1 at,$f0` hoist before the `addiu sp` prologue, and the test is
  `c.lt.s $f12,$f0` interleaved with the `sq` saves.
- All six int args are live across the branch, so EGC copies them into s0-s5 in
  the prologue (s0-s2 = cell1, s3-s5 = cell0). Path A's first call still uses the
  raw a0/a1/a2 (cell0) rather than s3-s4-s5.
- 0.5f is named `OCCL_CELL_FRACTION_MIDPOINT` (style: no magic numbers).

## Verification

- `tools/decomp_probe.py` vs the generated `.s`: 184/184 bytes, 0 diffs.
- `tools/tu_assembler_diff.py build/code/game/draw_post_post.o build/boot_elf.elf`:
  71/71 functions match.
- `make` + `cmp build/boot_elf.elf assets/boot_elf.elf`: byte-identical.

The callee `ParseOcclGrid` remains a blocked `INCLUDE_ASM` (see
draw_post_post_ParseOcclGrid.md); this function only calls it.
