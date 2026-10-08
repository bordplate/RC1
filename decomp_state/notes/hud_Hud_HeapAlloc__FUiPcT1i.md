# Hud_HeapAlloc__FUiPcT1i (code/game/hud.cpp) — matched 2026-10-08

## Semantics

Bump-allocates a 16-byte-aligned block from the HUD heap. Signature is
`(unsigned int size, char* comment, char* file, int line)` — only `size` is
used (the rest are the usual Insomniac `#`-assert source-tagging params,
unused at runtime).

Logic:
1. If `heapCursor == 0`, call `Hud_HeapReset()` (lazy init; the cursor field
   is 0 before any level has set the heap start).
2. If `size <= heapEnd - heapCursor` (SIGNED compare), hand out the OLD cursor
   as the block start and advance the cursor by `(size + 0xF) & ~0xF`.
3. Otherwise (heap full) return 0.

Globals are the same `HudHeap` struct + `hudHeapBase` used by
`Hud_HeapReset` (see notes/hud_Hud_HeapReset__Fv.md): cursor at +0x10, end at
+0x14. `HUD_RAM_ALIGN` (0x10) is the alignment constant already defined at
hud.cpp L5.

## Original body (vram 0x001FF288, 0x74 = 29 words)

```
addiu $29,$29,-0x30; lui $3,%hi(hudHeap); sq $17,0x10($29); sq $16,0($29)
addiu $17,$3,%lo(hudHeap); sq $31,0x20($29)
lw $2,0x10($17)            # s? cursor
bnez $2,.L1; daddu $16,$4,$0   # (delay) s0 = a0 (size)
jal Hud_HeapReset__Fv; nop
.align 2
.L1:
lw $2,0x14($17)            # end
lw $4,0x10($17)            # cursor
subu $2,$2,$4              # end - cursor
slt $2,$2,$16              # (end-cursor) < size   [SIGNED slt]
bnez $2,.L2; daddu $2,$0,$0   # (delay) v0 = 0 (return 0)
lui $3,0xFFFF              # v1 = 0xFFFFF000
addiu $5,$16,0xF           # a1 = size + 0xF
ori $3,$3,0xF0             # v1 = 0xFFFFFFF0 (mask)
daddu $2,$4,$0             # v0 = old cursor (result)
and $16,$5,$3              # s0 = (size+0xF) & ~0xF
addu $4,$4,$16             # a0 = cursor + aligned
sw $4,0x10($17)            # cursor = a0 (new)
.align 2
.L2:
lq $31,0x20($29); lq $17,0x10($29); lq $16,0($29)
jr $31; addiu $29,$29,0x30
# 3 trailing nops at 0x1FF2FC/0x1FF300/0x1FF304
```

Register map: s0=size→aligned, s1=&hudHeap, a0=old→new cursor, a1=size+0xF,
v0=result, v1=mask(~0xF), t0=(end-cursor) temp. Frame 0x30.

## Replacement (hud.cpp)

```cpp
char* Hud_HeapAlloc(unsigned int size, char* comment, char* file, int line) {
    register unsigned int sz asm("$16");
    sz = size;
    if (hudHeap.heapCursor == 0) {
        Hud_HeapReset();
    }
    if ((int)sz <= (int)(hudHeap.heapEnd - hudHeap.heapCursor)) {
        register u32 s15 asm("$5");
        s15 = sz + (HUD_RAM_ALIGN - 1);
        register u32 aligned asm("$16");
        aligned = s15 & ~(u32)(HUD_RAM_ALIGN - 1);
        register u32 cursor asm("$4");
        cursor = hudHeap.heapCursor;
        char* result = (char*)cursor;
        cursor += aligned;
        hudHeap.heapCursor = cursor;
        return result;
    }
    return 0;
}
// (3 x asm("nop") trailing padding follows, see findings #4)
```

## Codegen findings

1. SIGNED compare: the `size <= end - cursor` test is `slt` (signed), so both
   sides are cast to `int`. An unsigned compare would emit `sltu` (1-word diff
   in an otherwise identical function) — same signed/unsigned field lesson as
   readBufEndGet.

2. Four register pins, each proven load-bearing by removal (probe:
   working/hud_heapalloc/probeJ 0/0 word diffs; removing any one pin re-breaks
   it):
   - `sz`→$16 (s0): the original keeps `size` in s0 for the slt and then
     REUSES s0 for the aligned size; EGC otherwise hoists the aligned value
     into a different reg and drifts the whole `and`/`addu`/`sw` tail.
   - `s15`→$5 (a1): keeps `size+0xF` in a1 (the original's a1), not folded
     into the and.
   - `aligned`→$16 (s0): the aligned size must land back in s0 (reusing sz's
     slot) so `and $16,$5,$3` matches.
   - `cursor`→$4 (a0): the new cursor must be built in a0 (the original
     updates the old-cursor register in place: `addu $4,$4,$16; sw $4,...`).

3. Direct-return form: the body returns `(char*)cursor` (the OLD cursor loaded
   into v0) and a separate `return 0;`. A single top-level `char* result`
   assigned in both branches lets EGC CSE the v0 write and emit a
   `move v0,...` in the wrong slot; returning directly from each branch keeps
   v0 = old-cursor on the success edge and v0 = 0 on the fail edge.

4. TRAILING PADDING (shared-TU size, new finding): the original region is
   0x80 bytes — the 0x74 function PLUS three trailing nops (0x1FF2FC-0x1FF307)
   that the Splat `.s` emits after `endlabel`. Intra-TU `.text` addresses are
   cumulative source-order sizing, so a C body of 0x74 shrinks the region to
   0x78 (8-byte align) and shifts `func_001FF308` to 0x1FF300 AND every
   downstream `.data` code-pointer to the later hud functions by -0x8
   (~200 KB of data diffs) even though the function's own bytes match. Fix:
   emit the three trailing nops as bare file-scope `asm("nop")` after the C
   function (actuator.cpp:44-47 precedent). This is distinct from the
   ISOLATED-segment inter-function padding note (pause_setLevelSpriteList),
   where the section auto-pads to the segment size; a shared TU has no such
   fixed size, so the padding must be explicit.

## Verification (mechanical)

- Probe (working/hud_heapalloc/probeJ, GNU assembler, project flags):
  0/0 word diffs, 116 bytes (0x74).
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` PASSES.
- Raw 0x80-byte region at file 0xFF288 built == original (md5
  a238ab13266908c97348b9769788f77a); word at file 0xFF308 is `0f008b30`
  (`andi $11,$4,0xF`), i.e. func_001FF308 correctly lands at 0x1FF308.
- active count 501 → 500.

## Follow-ups

- `func_001FF308` (the next target, 0x10C bytes) reads `hudChanSlots`
  (`andi $11,$4,0xF; ... mult; ... lui %hi(hudChanSlots)`) — a texture-channel
  slot lookup; naming it will complete the HudHeap struct's +0x00/+0x04
  texture-cache-counter fields (see the Hud_HeapReset note's open item).
