# getFIFOindex (vibuf.cpp) — matched 2026-09-06

`u32 getFIFOindex(ViBuf* self, void* v)`, VMA 0x23BAF8, 68 bytes (0x44).
Converts a DMA address into a block index (block = 0x800 bytes) within the
ViBuf IPU ring, with a wrap special case:

```c
u32 getFIFOindex(ViBuf* self, void* v) {
    u32 i = ((self->blocks << 4) + self->tagBase + 0x10) & 0xFFFFFFF;
    if (v == (void*)i) {
        return 0;
    }
    return ((u32)v - self->base) >> 11;
}
```

## Semantics

- `base` (0x00): DMA base of the video-data ring.
- `tagBase` (0x04): DMA base of the tag array, one 0x10-byte tag per block
  (callers do `scTag2(self->tagBase + idx * 0x10, self->base + idx * 0x800,
  ...)` in viBufBeginPut/viBufAddDMA).
- `blocks` (0x08): ring size in 0x800-byte blocks (viBufCreate stores
  `capacity = blocks << 11` at 0x18).
- `i` = `tagBase + (blocks + 1) * 0x10` masked to 28-bit DMA space; when the
  incoming address equals that end marker the index wraps to 0, otherwise
  `(v - base) >> 11`.
- Callers: viBufBeginPut (0x23BF70, passes REG_DMAC_4_IPU_TO_MADR) and
  viBufAddDMA (0x23C280, passes self->dmac4ToMadr and a computed address).
  Results are used modulo `blocks` as tag-ring indices.

## ViBuf header renames

The pre-existing `code/include/vibuf.h` guessed `data` (0x00) and `chcr`
(0x04); both were wrong. Renamed per the evidence above: 0x00 `data` ->
`base`, 0x04 `chcr` -> `tagBase`. No other code referenced those names.
The 0x50 field is ALREADY named `tags` in the header (do not reuse the name);
0x50/0x54 are written by viBufCreate from videoDecCreate's trailing args and
remain uninvestigated.

## Codegen notes

- No stack frame; everything in registers. EGC layout rule (confirmed here):
  for `if (cond) A; B;` EGC 2.95.2 emits `branch-if-cond-to-A; B (fallthrough);
  A:`. The original has `beql $a1, $v1, L` (branch-if-equal), fallthrough = the
  `(v - base) >> 11` code, and `L` = return 0 at the end. That is exactly
  `cond = (v == i)`, `A = return 0`, `B = return expr`.
- The mirror form `if (v != i) { return expr; } return 0;` compiles to the
  opposite layout (`bnel`, return-0 as fallthrough) AND hoists the
  `lw base` into the branch delay slot (original keeps the delay slot a nop
  and the load in the fallthrough) — first 28 bytes match, tail diverges.
- Register mapping: self -> a2 (`daddu a2, a0`), mask const in v0 (lui/ori
  0xFFFFFFF), accumulator in v1, `tagBase` load reuses a0, `base` load lands in
  v0. All reproduced by the form above with DEFAULT flags (no per-file flags).
- `(u32)v - self->base` is 32-bit unsigned: EGC emits `subu` (64-bit op, low
  word correct) then `srl` (32-bit logical shift, zero-extends) with no
  explicit zero-extension instruction — matches the original exactly.

## Verification

- Standalone probe `decomp_state/probes/vibuf_fifoindex.cpp` vs generated
  reference: `match: true`, 0 differences (default flags).
- Final-ELF region 0x23BAF8..+0x44 byte-for-byte identical to the original.
- `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- decomp_status count 766 -> 765.
