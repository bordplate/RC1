# func_00200C80 (0x200C80, 0x184 = 97 words) — BLOCKED

## Purpose
VU1-chain packet appender. `void func_00200C80(int p1, int p2, int p3, int p4, int p5, int p6)`
— leaf, NO frame. Appends a 0x40-byte VU1 packet through the in-window global
`vu1ChainHead` (u32* @ 0x160F00):

```
head = *vu1ChainHead                       // old head
*head+0x00 = 0x10000003                    // 4 header words (each reloads the volatile global)
*head+0x04 = 0
*head+0x08 = 0
*head+0x0C = 0x50000003
*vu1ChainHead = head + 0x10                // advance
*(head+0x10) = 0x4400000000000001
*(head+0x18) = 0x4410
*(head+0x20) = 0x41
*(head+0x28) = p5                          // in the beqz DELAY slot (unconditional)
if (p6 == 0):  pack1 = pack(16*p1+minX-16, 16*p2+minY-16); pack2 = pack(16*p4+minX-16, 16*p3+minY-16)
else:          pack1 = pack(p1+minX-8, p2+minY-8);        pack2 = pack(p4+minX-8, p3+minY-8)
*(head+0x30) = pack1
*(head+0x38) = pack2
*vu1ChainHead = *vu1ChainHead + 0x30       // final, GPREL store in the jr $ra delay slot
```
`pack(x,y) = 0xFFFFF00000000000 | (u16)x | ((u64)(u16)y << 16)`; minX/minY =
occlViewParams.{minX,minY} @ +0x10/+0x14 (base 0x13E500). 4 call sites
(0x1FDAB4/AD4, 0x1FDB24/B48) confirm the 6-int ABI (5th in a4, 6th in a5).
Second pack is x=p4, y=p3 (NOT x=p3,y=p4 — an early candidate had these swapped).

## Walls and what was resolved
1. **Mixed-address vu1ChainHead — RESOLVED.** Original uses the in-window global
   as 7 self-based ABSOLUTE accesses (`lui %hi; lw/sw %lo(r)`) + 1 GPREL store
   (`sw r,-0x5D00(gp)`, final, in the jr delay slot). EGC emits any plain decl
   as small-data bare pseudos (one uniform mode). Fix (per last-resort, same as
   snd_BankLoadByLoc): declare `vu1ChainHead` double-volatile (bare pseudo → SN
   expands self-based absolute for the 7) and a second `vu1ChainHeadGp` alias at
   the SAME address with `.extern vu1ChainHeadGp,4` seeded at the TOP of the .s
   (so the final bare-pseudo store expands GPREL16). The candidate reproduces
   this mix exactly. NOTE: this requires an **SN-assembled TU** (the owning TU
   `hud_post_post2_post3.o` is GNU-assembled, which would expand every bare
   pseudo GPREL — wrong). A Splat boundary split + Makefile SN flag is required.
2. **p3/p4 swap — RESOLVED.** Second pack is x=p4, y=p3.
3. **Branch layout — RESOLVED.** `if (p6)` gives `beqz $9` (objdump-confirmed)
   with -8 as fall-through and the ×16 path as the branch target (matches
   original `beqz t1,0x200d78`). `if (p6!=0)`/`if(p6==0)` give the wrong opcode
   (beq/bne, r3000).

## Concrete blocker (60 word-diffs, best candidate cand3.cpp)
After the recipe, the header section (0x200C80–0x200CC0) and the prologue match.
All 60 remaining diffs are in the two pack branches (0x200D04–0x200E00), from:

- **Post-RA tail-merge CSE (the gate):** EGC merges BOTH branches' second-pack
  final `or r,r,y; or r,r,mask; sd r,0x38(base)` into ONE shared tail ($L6),
  reached by `b $L6` (fall-through path) and fall-through (×16 path). The
  original keeps a SEPARATE or/or/sd in each branch (different registers:
  -8 path $2/$5, ×16 path $4/$9). The branch target is therefore 2 instructions
  off (orig beqz→0x200D78, cand beqz→0x200D70), cascading through the whole
  data section. No C form prevents EGC 2.95.2 from merging the common tail: the
  two packs store the same value-shape to the same address, so the tail is
  structurally common. (A "compute both into locals then store" variant is
  worse — 81 diffs — because the original interleaves compute/store.)
- **Mask materialization:** original builds 0xFFFFF00000000000 as 32-bit
  `ori 0xFFFF; dsll 16; ori 0xF000; dsll 24` (per branch). EGC emits
  `dli 0xffff; dli 0xf000; dsll 16; or; dsll 24` (64-bit or with a
  separately-materialized 0xf000). The `asm volatile("" : "+r"(m))` barrier
  keeps EGC from folding to a single constant but forces the 64-bit `or` form
  instead of the 32-bit `ori`.
- **Scheduling:** occlViewParams base load / mask / p-arg sll interleaving
  differs from the original throughout both branches.

## Last-resort escalation
`last-resort-decompiler` (GPT-5.6 Sol) invoked 2026-10-11. Verdict MATCHABLE,
recipe: isolated SN TU + double-volatile vu1ChainHead + seeded GPREL alias
(`vu1ChainHeadGp`) for the final store + signed (s32) unmasked packing + staged
mask with `"+r"` barriers + direct repeated header accesses. Recipe applied; it
resolved the mixed-address wall and cut diffs 96→60, but the residue it
anticipated ("irreducible instruction-order/register-allocation residue") is
exactly the post-RA tail-merge CSE + mask/scheduling above, which the recipe did
not address and which is not controllable from C under EGC 2.95.2.

## Resume notes
If revisited: the mixed-address fix is proven (candidate .s shows the exact
7-abs+1-GPREL mix). The only structural gap is the second-pack tail-merge. A
possible (untested) lever: force the two packs to diverge before the final
or/or/sd so EGC cannot merge the tail — e.g. give the two packs different
register pressure via `register ... asm("$r")` pins, or emit the second-pack
store via a zero-byte `asm volatile("" : "+r"(w2))` tied barrier immediately
before each `sd`. None verified. Requires the SN-TU split (config/RC1.yaml
boundary, Makefile SN flag, config/linker_aliases.ld `vu1ChainHeadGp = 0x160F00`)
before the full build can be tested.
