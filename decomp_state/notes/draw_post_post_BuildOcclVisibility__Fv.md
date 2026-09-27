# BuildOcclVisibility__Fv (0x1F2820, 1008 bytes) — MATCHED

`void BuildOcclVisibility(void)` in `code/game/draw_post_post.cpp`.

## Semantics

Rebuilds the 128-byte `OcclVisibility` buffer (0x193FC0) used by the occlusion
system, then sets its validity flag `OcclVisibility[0x7F] |= 0x80`.

1. Compute the camera's octree cell `(cellX, cellY, cellZ) =
   (int)(currentCamera.pos * 0.25f)` per axis (`func_001FA6D0` = cvt.w.s,
   `func_001FA6C0` = cvt.s.w, both in `fastfunc.s`).
2. `mergedCell = ParseOcclGrid(cellX, cellY, cellZ)`. If found: clear
   `OcclInvalidGrid` (self-based store), copy the cell into `OcclVisibility`,
   record `OcclPreviousGrid = mergedCell` (GPREL store), done.
3. Otherwise set `OcclInvalidGrid = 1` (GPREL store). If `OcclMode == 0`: fetch
   the four neighbor cells via `GetOcclGridFromPair` (±1 in each axis; the
   fraction arg is `pos*0.25f - (float)cell`, computed in the JAL delay slot and
   passed in `$f12`). If any cell exists, zero `OcclVisibilityMerged`
   (0x194040) with `FastMemZero16` and OR-merge each found cell into it with
   `FastMemOr16` (newly named, 0x1F98F8: 128-bit `lq/lq/por/sq` loop), then
   `OcclPreviousGrid = OcclVisibilityMerged` (self-based) and copy to
   `OcclVisibility`.
4. If still no cell, switch on `OcclMode`:
   - 0: if `occlCamState.staged == 0 && OcclPreviousGrid != 0` copy the
     previous grid, else `FastMemSet(..., -1, 0x80)` (invalidate).
   - 1: invalidate.
   - 2: if `OcclOct` (0x15F644, float[3] table base) is set, pick the octant by
     sign of `currentCamera.pos - oct[n]` per axis (`octIndex = z + 2y + 4x`,
     each test `pos - oct[n] > 0.0f`) and copy
     `OcclOct + octIndex*0x80 + 0x10`; else the mode-0 fallback.
5. Set the validity flag.

Sole caller: `UpdateOcclusion` (0x1F2C10) when `OcclUpdate == 2`.
`occlCamState.staged` (0x18C32C) has 10 read xrefs and zero write xrefs in the
boot ELF — level overlays write it when staging a camera transform.

## Match-critical details

### The staged-flag load: struct field + v1 pin

The original reads the staged flag as a genuine two-register split:
`lui v0,0x19; lw v1,-0x3CD4(v0); bnez v1` (case 0 at 0x1F2A90/94 and
case-2-else at 0x1F2B84/88). The 0x190000 hi page stays live in v0 across the
test and is REUSED by the same-page copy destination
(`addiu a0,v0,16320` = 0x193FC0 at 0x1F2BAC).

Three independent requirements produce that form:
- Reading the **struct field** `occlCamState.staged` (not the scalar
  `occlCamStaged`): a 4-byte scalar extern is small data under `-G8` and
  compiles to a single bare pseudo `lw $v0,occlCamStaged` with no base-register
  RTL; ps2eeas expands it self-based (`lui v0; lw v0,off(v0)`), which cannot
  become the two-register split. (8-byte objects are small data too; only
  struct fields / >8-byte / `.data`-section forms force the split.)
- **Pinning** the value to v1: `register int staged asm("$3");` with
  `staged = occlCamState.staged;`. Without the pin EGC unifies base and value
  into v0 anyway (a legal `lw r,off(r)` move-pattern overlap) — 4-word diff.
  Pinning the value to v1 forces base v0 / value v1.

The self-based form is not just a cosmetic diff: on the case-0 copy path the
last v0 write before `addiu a0,v0,16320` would be the staged value (0),
corrupting the destination to 0x3FC0. The two-register design exists so the
page survives.

### Mixed addressing: Gp aliases

`OcclPreviousGrid` (0x15F650) and `OcclInvalidGrid` (0x15F64C) are written
GPREL16 in the first-branch/mode-0 setup but read back self-based absolute in
the switch. Same recipe as `partClipDistGp`/`screenFadeGp`: plain symbol in
`config/symbols.txt` (self-based) plus a same-address `.extern`-seeded alias
in `config/linker_aliases.ld` for the GPREL sites.

### Leftover-v0 alias for GetOcclGridFromPair

`GetOcclGridFromPair` (matched, 0x1F2768) returns void but ends with a
`ParseOcclGrid` call that leaves the cell pointer in v0. The C form
`char* getOcclGridFromPairCell(...) asm("GetOcclGridFromPair__Fiiiiiif")`
reads that leftover; the fraction is the 7th arg in `$f12` (standard first
float arg — the callee compares it against its own 0.25 constant).

### Octant tests

`octX = currentCamera.pos - oct[0] > 0.0f;` (and y/z). Micro-probe verified:
only this form emits `sub.s + mtc1 $zero + c.lt.s + bc1t` like the original;
`>=`, `!(a < b)` and `a - b >= 0.0f` all emit `c.le.s`.

## Rename

`func_001F98F8` (48-byte 128-bit OR-merge loop in `fastfunc.s`, no name in the
original symtab) renamed to `FastMemOr16__FPvN20i` in `config/symbols.txt`,
following the `FastMem*` family (`FastMemZero16__FPvi` is its sibling).
`cfront` mangles `FastMemOr16(void*,void*,void*,int)` as
`FastMemOr16__FPvN20i` (repeated param types as `N20`, not `T0T0`) — verified
with a standalone stub compile.

## Verification

`tools/decomp_probe.py` on the standalone candidate: 1008/1008 bytes, 0 diffs
(working/buildoccl/out21). Full `make` + `cmp build/boot_elf.elf
assets/boot_elf.elf` clean.
