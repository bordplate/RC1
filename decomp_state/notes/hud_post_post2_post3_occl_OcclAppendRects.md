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
