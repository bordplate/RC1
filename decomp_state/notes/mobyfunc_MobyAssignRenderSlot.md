# MobyAssignRenderSlot (func_0020CC18, 0x20CC18, 0x44 bytes)

Matched 2026-10-02. Byte-for-byte, full boot parity passes. Count 590 -> 589.

## Semantics

`int MobyAssignRenderSlot(MobyInstance* m)` claims a moby render slot:

- `mobyRenderSlots` (renamed from `D_001B2BC0`) is a 16-slot pool of
  `MobyInstance*` at 0x1B2BC0 (data section, outside the GP window). A
  per-slot vec4 table follows immediately at 0x1B2C00 (16 x 0x10 bytes).
- Scan slot `count` = 0..15: if the slot is empty (`*p == 0`) or already holds
  `m` (`*p == m`), store `m` there and return `count`.
- If all 16 slots hold other mobies, return -1.

Both callers (`func_00212F90`, `func_002130D8` in mobyutil, still INCLUDE_ASM)
use it identically: on a valid index they call `FUN_0020ede8(m, index|flags)`,
copy a vec4 from the moby into the 0x1B2C00 table at `index*0x10`, set
`MobyInstance+0x52 = 0xFF` (sentinel) and `MobyInstance+0x50 = index` (the slot
index field, `field26_0x50`). The twin `func_0020CC60` (still INCLUDE_ASM)
clears dead slots in the same pool by testing `MobyInstance+0x20 & 0x80` and
`MobyInstance+0x52 == 0xFF`.

## Matching form

Clean for-loop, no register pins, no private flags:

```cpp
#define MOBY_RENDER_SLOT_COUNT 16
extern MobyInstance* mobyRenderSlots[MOBY_RENDER_SLOT_COUNT];

int MobyAssignRenderSlot(MobyInstance* m) asm("func_0020CC18");
int MobyAssignRenderSlot(MobyInstance* m)
{
    int count;
    for (count = 0; count < MOBY_RENDER_SLOT_COUNT; count++) {
        MobyInstance** p = mobyRenderSlots + count;
        if (*p == 0 || *p == m) {
            *p = m;
            return count;
        }
    }
    return -1;
}
```

## Codegen notes

- The two-branch `if (*p == 0 || *p == m)` is the key. The zero case and the
  match case both fall into ONE shared return label (`jr $ra; daddu $2,count`),
  with the mismatch case continuing the loop. Writing the match case as a
  separate `if/else` (store in the else) made EGC emit an extra inline return
  (`j $ra; move`) and grow the function to 0x48.
- Prologue: `lui $2, %hi(mobyRenderSlots); daddu $3,$0,$0; addiu $5,$2,%lo` --
  the pool base `lui` lands in $2 (scratch) and the pointer result in $5 (a1),
  count in $3 (v1). This happens naturally from the for-loop induction. Pinning
  count to $3 while pinning the pointer to $5 forced the `lui` into $5 (2 diffs);
  pinning only count, or letting the for-loop allocate, both give the original.
- `mobyRenderSlots` is outside the GP window, so a plain extern array reference
  emits the absolute self-based `lui; addiu` load (no section attribute needed).
- EGC emits the return as `j $ra` in the .s text; ps2eeas assembles it to
  `jr $ra` (0x03E00008) -- same bytes as the original. Verified via the probe
  byte comparison (original_size == candidate_size == 68, sha256 present).

## Symbol / linkage

The boot ELF is stripped, so the original name is unknown. Splat's placeholder
`func_0020CC18` is the link target the still-assembly callers reference, so the
definition keeps that symbol via an asm override (declaration-with-asm-label
then bare definition, per the camera.cpp `ExecuteCamPostUpdFuncs` pattern; a
same-line `) asm("...") {` fails to parse in this EGC).

## Renames

- `D_001B2BC0` -> `mobyRenderSlots` in config/symbols.txt (propagated by
  `make split` to func_0020CC18.s and func_0020CC60.s).
