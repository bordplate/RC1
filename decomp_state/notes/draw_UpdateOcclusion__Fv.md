# UpdateOcclusion__Fv (0x1F2C10, 80 bytes)

Matched 2026-09-07 with default flags (draw.cpp TU flag).

```cpp
void BuildOcclVisibility(void);

extern "C" int OcclUpdate __attribute__((section(".data")));
extern "C" char OcclVisibility[] __attribute__((section(".data")));

void UpdateOcclusion() {
    if (OcclUpdate == 0) {
        FastMemSet(OcclVisibility, -1, 0x80);
    } else if (OcclUpdate == 2) {
        BuildOcclVisibility();
    }
}
```

## Semantics

Occlusion-update dispatcher keyed on the global `OcclUpdate` (0x18C334, out of
the gp window):
- `0`  -> fill the occlusion-visibility grid `OcclVisibility` (0x193FC0, 0x80
  bytes) with -1 via `FastMemSet(void*, int, int)` (0x1F97E8).
- `2`  -> rebuild the grid via `BuildOcclVisibility` (0x1F2820, same TU).
- otherwise do nothing.

void return; the two callers (draw dispatch) ignore any return value.

## Codegen — the symbol-vs-cast scheduling flip (the crux)

This function's original schedule is unusual and EGC only reproduces it with
SYMBOL-based data access, not constant-address casts:

```
lui    v0, %hi(OcclUpdate)     # hoisted BEFORE the prologue
addiu  sp, sp, -0x10
lw     v1, %lo(OcclUpdate)(v0) # base in v0, value in v1
bnez   v1, .Lelse
  sq     ra, 0(sp)             # prologue store in the bnez DELAY slot
lui    a0, %hi(OcclVisibility)
li     a1, -1
addiu  a0, a0, %lo(OcclVisibility)
jal    FastMemSet
  li     a2, 0x80
b      .Lend
  lq     ra, 0(sp)
.Lelse:
  li     v0, 2                 # deferred to the else block
  bne    v1, v0, .Lend
    lq     ra, 0(sp)
  jal    BuildOcclVisibility
    nop
  lq     ra, 0(sp)
.Lend:
  jr     ra
  addiu  sp, sp, 0x10
```

With constant casts (`*(int*)0x18C334`, `(void*)0x193FC0`) EGC instead:
keeps `addiu sp; sq ra` together up front, allocates the load base to **v1**
(reusing v1 for both `lui` base and `lw` dest), and puts `li v0,2` in the bnez
delay slot. None of the four differences (base register, sq-ra placement,
li-v0,2 placement, lui hoist) could be flipped back with C restructuring
(local var vs. inline, nested if) — the constant-cast form is stuck.

Declaring the two out-of-window globals as `extern ... __attribute__((section
(".data")))` and using the named symbols makes EGC emit the original schedule
exactly: it keeps the load base in v0 (separate from the v1 value), defers
`sq ra` into the bnez delay slot, defers `li v0,2` to the else block, and
hoists `lui` before `addiu sp`. This mirrors the matched Hud functions
(Hud_HeapReset) that access out-of-window .data symbols by name.

No matched precedent hoists a `lui` before a prologue, so treat "symbol access
schedules differently than a constant cast" as the reusable takeaway, not the
hoist itself.

## Verification

Clean `make clean && make split && make -j2`; `cmp build/boot_elf.elf
assets/boot_elf.elf` passes. Relocations resolve to OcclUpdate=0x18C334,
OcclVisibility=0x193FC0, FastMemSet=0x1F97E8, BuildOcclVisibility=0x1F2820.
