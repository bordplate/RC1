# ParseOcclGrid (0x1F2690) — BLOCKED

## Summary
`char* ParseOcclGrid(int x, int y, int z)` (0xD4 bytes) in `code/game/draw_post_post.cpp:83`.
Three-level occlusion-grid lookup; returns `pgrid + (e3<<7)` or 0. First half (L1 + L2
offset/count) matches byte-for-byte in the flat form. The second half cannot be matched:
three independent EGC 2.95.2 codegen decisions are mutually in tension, and no source form
or flag tested satisfies all three.

## Semantics (verified vs objdump, Lombyte M2C, Deadlocked)
- `base = *(u32*)occlFlag0` (global 0x15F640); `pgrid = base + *(u32*)base` (t1).
- Levels (z, y, x): `r = arg - node.offset`; return 0 if `r<0` or `r>=node.count`;
  `e = node.children[r]`; return 0 if `e==0` (L1/L2) or `e==0xFFFF` (L3).
- Node = `{u16 offset; u16 count; u16 children[]}`; root children at base+8; next = `base + e*4`.
- Regs: t0=base, a3=node, v0/v1 temps, a2=z, a1=y, a0=x, t1=pgrid.

## The three tensioned codegen decisions
1. LAYOUT (ret0 placement): the original places the merged 8-way early-return block (`ret0`)
   right after the L2 child (e2) test — inverting that test to `bnezl` (reload in the delay
   slot), with the whole L3 body AFTER ret0 and the x-range tests branching BACKWARD to ret0.
   The e1 child test is NOT inverted (plain `beqz`, nop delay slot, reload in fallthrough).
   EGC 2.95.2, for the flat form, instead places ret0 after the L3 count (x>=count) test and
   inverts THAT. Only an all-`goto` form (shared `reject` label lexically between the e2 test
   and the L3 body) reproduces the original layout.
2. E1 CHILD (reload vs move): the original RELOADS e1 (`lhu v0,4(v0)` in the fallthrough, nop
   delay slot). The all-goto form that fixes the layout instead KEEPS e1 in v1 and emits
   `move v0,v1` in the delay slot (no reload) — 4 bytes shorter. The flat form (which reloads
   e1 correctly) has the wrong layout.
3. TAIL (0xFFFF compare): the original computes `addu v0,v0,a3` (children addr) FIRST, then
   `li a0,0xffff` (constant in A0 — the dead rx register), `lhu v1,4(v0)` (e3 in V1),
   `beq v1,a0`, then `move v0,v1` in the beq delay slot + `sll v0`. EVERY candidate (flat and
   goto) instead loads e3 directly into V0 and puts the constant in V1 (`li v1,0xffff` before
   the addu, `beq v0,v1`, no move). No form reproduced the original's a0/v1 + move tail.

## Forms tested (probe: tools/decomp_probe.py vs the generated .s, symbol ParseOcclGrid__Fiii)
- v2 flat separate-ifs: 24 diffs (best; correct e1 reload + size, WRONG layout + tail).
- v3 nested `if(e2){L3}return 0;`: 29 (wrong grouping: e2==0 becomes the separate tail).
- v4 combined `||` per level: 33 (also disturbs the matching first half).
- v5 u16 grid_index: 24 (identical to v2).
- v6 short indices: 59 (truncation/extension code).
- v7 store child in local var: 42 (removes the double-load the original does).
- v8 all-goto spelling A (`if(e2)goto level3;`): 39.
- v9 all-goto spelling B (`if(!e2)goto reject; goto level3;` + top-declared scalars): 39 —
  CORRECT layout (e2 inverted, ret0 after e2, x-tests backward) but e1 becomes a move and the
  tail is still wrong.
- v10 hybrid (flat L1-2, single goto at e2, flat L3): 24 — EGC canonicalizes the lone goto
  away; byte-identical to v2 (does NOT fix the layout).
- v12 v9 + `int shifted = grid_index<<7` temp: 39 (no change).
Flags on v2/v9: -fno-schedule-insns 27, -fno-schedule-insns2 43, -O1 61, -O3 24, -Os 43,
-fno-thread-jumps 24/39. None match.

## Last-resort escalation
`last-resort-decompiler` (GPT-5.6 Sol) was invoked 2026-09-27 with the full dossier. Its
concrete recommendation was the explicit shared-`reject`/`level3` goto form (both spellings)
plus, if needed, `-fno-thread-jumps`. All three were implemented and mechanically tested:
both goto spellings give the correct block layout but introduce the e1 move (vs the original's
reload) and leave the a0/v1+move tail unreproduced; `-fno-thread-jumps` changes nothing.
The agent itself concluded that if the lexical-order-matching CFG still canonicalizes to the
wrong layout/RA, it is an irreducible EEGCC jump/layout pass behavior.

## Conclusion
Irreducible EEGCC 2.95.2 block-layout + register-allocation tie-break. The original's exact
combination (ret0 at the L2/L3 boundary with e2 inverted, e1 reloaded not moved, and the
0xFFFF constant in a0 with e3 in v1 + a move) cannot be reproduced by any C source form or
flag found. Kept as INCLUDE_ASM (generated .s references the named symbols; overlay-safe).
See working/parse_occl_grid/NOTES.md and out_*/candidate.json for the per-variant diffs.
