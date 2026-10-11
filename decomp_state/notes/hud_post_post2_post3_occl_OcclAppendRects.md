# OcclAppendRects (func_00200E08, 0x200E08, 0x184 = 97 words) — MATCHED

## Purpose
VU1-chain packet appender, sibling of the blocked `func_00200C80`.
`void OcclAppendRects(int x0, int y0, int x1, int y1, u64 attr, int scale)`
— leaf, NO frame. Appends a 0x40-byte (12-word) inline VU1 packet to the chain
through the in-window global `vu1ChainHead` (u32* @ 0x160F00):

```
header (each re-reads the volatile global, self-based absolute):
  *vu1ChainHead+0x00 = 0x10000003   (VU1_INLINE_PKT_TAG | 3)
  *vu1ChainHead+0x04 = 0
  *vu1ChainHead+0x08 = 0
  *vu1ChainHead+0x0C = 0x50000003   (VU1_DATA_REF_END_TAG | 3)
head = *vu1ChainHead
*vu1ChainHead = head + 0x10         // intermediate advance (GPREL via alias)
*(head+0x10) = 0x4400000000000001   // payload stores through the OLD head
*(head+0x18) = 0x4410
*(head+0x20) = 0x46
*(head+0x28) = attr                 // in the beq DELAY slot (unconditional)
if (scale): pack1 = pack(x0+minX-8, y0+minY-8);   pack2 = pack(x1+minX-8, y1+minY-8)
else:       pack1 = pack(16*x0+minX-16, 16*y0+minY-16); pack2 = pack(16*x1+minX-16, 16*y1+minY-16)
*(head+0x30) = pack1                // in the `b` (unconditional) DELAY slot
*(head+0x38) = pack2
*vu1ChainHead = *vu1ChainHead + 0x30 // final, GPREL store in the jr $ra delay slot
```
`pack(x,y) = 0xFFFFF00000000000 | (u16)x | ((u64)(u16)y << 16)`; minX/minY =
occlViewParams.{minX,minY} @ +0x10/+0x14. 38 call sites confirm the ABI.

## ABI (definitive)
6 params; the EGC register-arg window here is a0-a3, t0-t3 (NOT a0-a5+stack):
`(x0,y0,x1,y1)` in a0-a3, the `u64 attr` in t0/t1, and the 6th int `scale` in
t1 (overlapping the u64 high word). The branch word `0x1120001c` (EE objdump:
`beq t1,zero`) takes the ×16 path when `scale==0`; the raw path is the
fall-through. `sd t0,40(t2)` stores the whole (t1:t0) u64 pair. 5-arg forms
(`attr>>32`, `>0xFFFFFFFF`, `&mask`) all failed (87-104 diffs).

## The two walls and their resolution
1. **Delay-slot stores need a NON-volatile head.** The payload stores
   (`*(u64*)&head[N]`) go through a PLAIN (non-volatile) `u32* head =
   (u32*)vu1ChainHead;` local, NOT a volatile one. EGC's delay-slot path will
   not pull a VOLATILE store into a branch delay slot; the original schedules
   the attr store into the `beq` delay slot and the second-rect store into the
   unconditional `b` delay slot. Making `head` volatile left exactly those two
   slots as nops (40 word-diffs: the 2 misplaced stores + a 4-byte cascade).
   This is the same plain-`head` form as the matched `Hud_sendTexture`.
2. **SN-assembled TU (the -40 byte shift).** The owning TU
   `hud_post_post2_post3.o` is GNU-assembled, which expands every bare
   `lw/sw r, vu1ChainHead` pseudo to a single GPREL16 — my function then
   compiled to 0x160 (352 B) instead of 0x184 (388 B), shifting every later
   forward pointer (e.g. the 0x206xxx table at vram 0x19FEBC) by -0x28 and
   breaking full parity. The double-volatile `vu1ChainHead` needs ps2eeas to
   expand each bare pseudo to a fresh self-based lui/lw pair. Fix: Splat
   boundary split isolating this function into its own SN-assembled TU
   (`hud_post_post2_post3_occl`, config/RC1.yaml @ 0x100e08; NOT in the Makefile
   GNU list). The region is 0x188 (function 0x184 + one word of inter-function
   padding), so a single file-scope `asm("nop")` after the function fills the
   gap (actuator.cpp precedent). The tail functions moved to
   `hud_post_post2_post3_tail` (GNU, unchanged codegen); `func_00200C80` stays
   in `hud_post_post2_post3` (GNU).

## Verification
- Standalone: `tools/decomp_probe.py` → `match: true`, 0 diffs, 388 bytes.
- Full build: `make split && make -j2; cmp build/boot_elf.elf
  assets/boot_elf.elf` → EXACT MATCH (all C/C++ objects forced-rebuilt by the
  Makefile change). Object size confirmed 0x184 via nm; 0x206xxx pointers realigned.
- Mangled name `OcclAppendRects__FiiiiUli` verified via nm (cfront spells
  `i i i i U l i`, no Tn refs). Named in config/symbols.txt = 0x00200E08.

## Refactor (2026-10-12) — payload magic named
The 48-byte payload after the 4-word tag block is NOT VU1 code (the earlier
"GSGX/ADDA/LQ" read was a misread). It is the engine's GS solid-color sprite
(rectangle fill) record, streamed to the GS by the VU1 chain:

```
+0x10  16-byte GIFTAG:  [0x00000001, 0x44000000, 0x00004410, 0x00000000]
                        qwc byte 1, 0, 0, type byte 0x44 (solid sprite),
                        param word 0x4410
+0x18  16-byte attribute block: [0x46, 0x00000000, attr, 0x00000000]
                        lead word 0x46 (meaning not established), then the
                        caller's attr word (an ARGB color/attribute value:
                        0x80000000, 0x80f0f0f0, 0x20959544... at call sites)
+0x20  16-byte corner block: [pack(c0), pack(c1)]
                        pack(x,y) = x | (y<<16) | 0xFFFFF000000000
                        = the rect's two packed corners (top-left -> bottom-
                        right), NOT two rects: all 38 callers pass one
                        rectangle per call (e.g. fullscreen 0,0,0x200,0x1C0;
                        text-box 0x75-w,0x6d,0x7b,0x70)
```

Meaning confirmation: the boot ELF keeps the engine's static GIFTAG table at
0x160820-0x160858 — [0x8001, 0x24000000, 0x10, 0], [0x8002, 0x14000000, 0x4,
0], [0x8001, 0xb4000000, 0x35353106, 0x535], [0x8001, 0xb4000000, 0x25252106,
0x525] — the same [qwc|0x80, type-page, param, pad] layout with sibling types
0x24/0x14/0xB4 (the 0x44 tag itself is only built at runtime by this family).
The Deadlocked reference (reference/dl/game_dl/tables.cpp GIFtag_PrimPixel =
[0x4400000000008001, 0x4410]) is exactly this tag with its 0x80 flag byte
set, and Deadlocked DrawSprite (reference/dl/game_dl/draw.cpp) packs corners
identically (`x | (y<<16) | 0xfffff000000000`) with x16-scaled coordinates.

The tag qcnt (3) counts the three payload blocks. `attr` is the color/
attribute word; `scale` selects integer-pixel input (scaled x16, corner
offset -0x10) vs 16.0 fixed-point input (raw, corner offset -8);
occlViewParams.minX/minY are pre-scaled x16 by InitViewContext (<< 4), so
both paths land in the same 16.0 fixed-point corner space. The 0xFFFFF000
high word of each packed corner is taken from the caller instead in the
7-arg sibling func_00200F90 (param_6).

Named via #defines + codegen-neutral text macros (OCCL_RECT_GIFTAG,
OCCL_RECT_GIFTAG_PARAM, OCCL_RECT_ATTR_LEAD, OCCL_RECT_CORNER_HIGH,
OCCL_RECT_X16_SCALE, OCCL_RECT_SCALED_CORNER_OFFSET, OCCL_RECT_RAW_CORNER_
OFFSET, OCCL_RECT_PACK_RAW, OCCL_RECT_PACK_X16) — macro expansion folds to
the same literal values and token layout, so codegen is unchanged.
Verification: incremental rebuild + region dump 0x200E08..0x200F8F
byte-identical, then clean `make clean + make split + make -j2` and
`cmp build/boot_elf.elf assets/boot_elf.elf` → EXACT MATCH.
