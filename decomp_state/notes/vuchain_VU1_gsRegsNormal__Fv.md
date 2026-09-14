# VU1_gsRegsNormal__Fv (0x233BC8, 0x5C) — MATCHED 2026-09-14

Appends the VU1 packet that streams the normal GS register state block to the
VU1: `[0x30000003, 0x1DE3C0, 0, 0x50000003]` at the chain head, then advances
`vu1ChainHeadStore` to head+4. Data block `vu1GsRegsNormal` (0x1DE3C0, 12
words) named in config/symbols.txt. One-shot callers in draw.cpp
(FUN_001f39d0, 0x1F3B58/0x1F3C2C) stream normal vs special GS state depending
on a flag. Same family as matched VU1_addDataRef__FPvi (see its note for the
`vu1ChainHead` / `vu1ChainHeadStore` declaration recipe, reused unchanged).

## Solution

`code/game/vuchain.cpp`, TU flag `-mno-split-addresses` (vuchain.o):

```cpp
void VU1_gsRegsNormal() {
    volatile u32* packet = vu1ChainHead;
    asm volatile("" : : "r"(packet));
    u32 tag = VU1_DATA_REF_TAG | 3;
    asm volatile("" : : "r"(tag));
    u32 address = 0x001E0000;
    asm volatile("" : "+r"(address) : "r"(packet), "r"(tag));
    packet[0] = tag;
    address -= 0x1C40;
    vu1ChainHead[1] = address;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 3;
    vu1ChainHeadStore = vu1ChainHead + 4;
}
```

The three zero-byte asm barriers are a documented codegen exception (no
machine instructions are emitted). They are required to reproduce the
original's interleaved order
`[head1][tag hi][tag lo][addr hi][store0][addr lo][end hi][head2]...`:

1. `address = 0x1E0000; address -= 0x1C40;` must NOT constant-fold. EGC folds
   pure-constant initializer+subtraction to `li 0x1D; ori 0xE3C0` (UNSIGNED
   split) — 2-word diff. The `"+r"(address)` barrier keeps them two separate
   RTL insns; the subtraction is a negative-immediate `addiu -0x1C40`, i.e.
   the original's SIGNED split (hi 0x1E + lo -0x1C40). 0x1E0000 is the
   high-16 split page of 0x1DE3C0 (genuine codegen artifact, no symbol).
2. Barrier 1 pins the head1 load before the tag's RTL definition: with the
   folding barrier present, the CSE pass hoists the tag's constant definition
   ahead of the head load (tag first = 3-word diff).
3. Barrier 2 pins `tag lo` before `address hi`: otherwise the pre-RA
   scheduler swaps the independent pair (2-word diff).

Input-only barriers (`"" : : "r"(x)`) pin ordering without disturbing register
allocation; the `"+r"(packet)` form shifted head1 from a0 to v1 and broke the
match (12-word diff).

## What does NOT work (probe matrix, all 0x233BC8)

- Natural body (named symbol or raw constant at slot 1): with the flag, a
  symbol address compiles to ONE atomic `la` pseudo (unsplittable, never
  interleaves around store0) — 16-word diff; a raw constant folds to the
  unsigned split — 2-word diff. Without the flag, symbol addresses RTL-split
  signed (interleavable) but the five head-pointer loads CSE into one shared
  base register (two-register loads) — 22-word diff.
- No flag combination on the natural body: none, -fno-schedule-insns,
  -fno-schedule-insns2 (13), both, -fno-regmove, -fno-expensive-optimizations,
  -O1, -Os, -O3 (2 with constant addr).
- Declaration kind for head (plain/sized/const/volatile `.data`, function
  type) and for data (plain/sized/const/section attr, offset forms): all
  land in one of the two failure modes above.
- Reference the data through the named symbol with constant arithmetic
  (`(u32)(vu1GsRegsNormal + 0x1C40) & 0xFFFF0000` etc.): EGC does NOT
  constant-fold expressions containing data-symbol addresses (only
  pure-constant expressions fold) — 24-word diff.
- Five distinct `.data` alias symbols at 0x160F00 (no flag): head loads are
  separate lui pairs but two-register (no coalescing) — 19-word diff.
- Barrier variants: `"+r"(packet)` after head1 (12), single folding barrier
  only (3), no barrier (2, folded constant).

## Family

func_00233C28 (0x233C28): same packet shape, data 0x1DE3F0 (lo 0xE3F0 ≥
0x8000 — also needs the signed-split barrier trick), likely a shared original
macro with 0x30000003/0x50000003 vs 0x3000000B/0x5000000B tags.
func_00233C90 (0x233C90): tags 0x3000000B/0x5000000B, data 0x13CF10 (lo
0xF10 < 0x8000 — unsigned fold may suffice, untested). Start from this note's
form when tackling either.
