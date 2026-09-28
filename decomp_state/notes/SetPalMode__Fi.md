# SetPalMode__Fi (0x001F34E8, 0x300 bytes) — investigation notes

## What it does
Selects PAL/NTSC buffer layout, recomputes the occlusion view rectangle,
programs the font-pipeline GS registers (vu1GsRegsFont) + sky reference words
+ texture cursor, then uploads z-buffer 32x32 tiles into loadImageBuffer.

## Address map (verified)
- Branch var = videoModePal @ 0x15ED80 (symbols.txt:498). `if (videoModePal != 0) PAL else NTSC`.
- displayBase @ 0x15EE80 (zeroed both branches); zbufBase @ 0x15EE84;
  frameBufferBase @ 0x15EE88; textureMemoryBase @ 0x15EE8C.
- textureCursor @ 0x15EE74; textureCursorEnd @ 0x15EE78.
- skyRefTextured @ 0x13D100; skyRefGouraud @ 0x13D170.
- vu1GsRegsFont @ 0x13CF10 (fontRegs, u64 stores at +0x10/0x20/0x30/0x40/0x50/0x60/0x70/0x80).
- loadImageBuffer @ 0x1941C0.
- PAL: texmem 0x2C0000, zbuf 0x100000, fb 0x1E0000, SetupFS_AA_buffer(0x200,0x1C0,0x200,0x200,4,0).
- NTSC: texmem 0x280000, zbuf 0xE0000, fb 0x1B0000, SetupFS_AA_buffer(0x200,0x1A0,0x200,0x1C0,0,0).

## OcclViewParams layout (verified from store block)
offset 0x0 paramX, 0x4 paramY, 0x8 halfX, 0xC halfY, 0x10 minX, 0x14 minY,
0x18 maxX, 0x1C maxY. OCCL_VIEW_CENTER = 0x800.
minX=(0x800-halfX)<<4, minY=(0x800-halfY)<<4, maxX=(halfX+0x800)<<4, maxY=(halfY+0x800)<<4.
paramX=(s16)drawW, paramY=(s16)drawH, halfX=(s16)drawW>>1, halfY=(s16)drawH>>1.
drawW @ occlCamParamBase+0x150, drawH @ +0x152 (u16); zbufW @ +0x15A, zbufH @ +0x158 (s16).

## Semantic fix (important)
fontRegs[10] = minX | (u64)minY   (minX raw lw, minY zero-ext)
fontRegs[12] = (u64)minX | (u64)minY   (both zero-ext)
NOT (minY<<32)|minX. The machine does `or $12,$12,$8` / `or $11,$11,$7` — a plain
OR, no shift. Original redundantly zero-extends: daddu $7,$8; dsll32 $8; daddu $11,$12.

## MATCHES
- Prologue (0xD0 frame, 7 saves at 0x60..0xC0, 16-stride).
- occlViewParams block (the InitViewContext-form source matches).
- z-buffer tile loop: tiles=(zbufW*zbufH)>>10; arg i*16 (sll20+sra16);
  body sceGsSetDefLoadImage(&image,i*16,1,0,0,0,32,32); FlushCache; sceGsExecLoadImage(&image,loadImageBuffer); sceGsSyncPath.
- EGC 8-arg ABI: a0-a5, t0-t3.

## BLOCKER: register-allocation wall (fontRegs block + branch stores)
Original register allocation (fontRegs block):
  a0=0x1000000, a1=base(fb), a2=paramX(->paramX>>6), v0=paramY, v1=vu1GsRegsFont,
  t0=minY, t1=zbuf(->zword), t2=paramX-1(->transfer), t3=minX, t4=minX(->scis), t5=texmem.
Candidate (EGC 2.95.2, no pins):
  v0=0x1000000, a1=vu1GsRegsFont, a2=paramX, a3=base, a0=paramX>>6,
  t0=zbuf, t1=minY, t2=minX, t3=texmem.  (a-regs preferred, t4/t5 unused, a3 used)

With pins fontRegs->v1/$3 and base->a1/$5, the a/v split matches (v0=paramY,
v1=vu1GsRegsFont, a1=base, a2=paramX) but t-registers + a0 still differ and the
scis is CSEd (1 OR vs original's 2 ORs + 3 zero-exts) -> function 0x2e8 (6 instr
short of 0x300). Raw binary then misaligns (objcopy -O binary -> 16 B shorter).

Branch stores: original a1=texmem,v0=zbuf,v1=fb storing texmem,zbuf,fb;
candidate a1=fb,v0=texmem,v1=zbuf storing fb,texmem,zbuf (3-cycle rotation).

## Attempted
- C reordering (base before/after transfer/zword) — failed.
- Explicit locals pxm1/pym1/pxs6 — failed.
- 4 pins (base->a1, fontRegs->v1, transfer->t2, zword->t1) — WORSE (116 diffs, misaligned).
- 2 pins (base->a1, fontRegs->v1) — improved a/v, still wrong t-regs + size (0x2e8).
- scis <<32 removal (semantic fix) — needed.
- **Expert (once)**: recommended a zero-byte tied read/write asm barrier to
  break scis equivalence before widening. APPLIED: `u32 scisX10/12, scisY10/12`
  + `asm volatile("" : "+r"(scisY10),"+r"(scisX12),"+r"(scisY12))` then
  `fontRegs[10]=(u64)scisX10|(u64)scisY10; fontRegs[12]=(u64)scisX12|(u64)scisY12`.
  Result: two separate ORs + zero-exts (dsll32+dsrl32) now match the original's
  structure; function grew 0x2e8 -> 0x2f8.
- **Scis-register pin**: `register u64 scis10 asm("$12"); register u64 scis12
  asm("$11");` then store. APPLIED: scis stores now in t4/t3 (matches original's
  `sd $12`/`sd $11`). Size still 0x2f8.

## CURRENT STATE (checkpoint: SetPalMode_scis_pin.cpp)
- 0x2f8 (6 instr short of 0x300). Scis structure + scis registers (t4/t3) match.
- Remaining wall: PROLOGUE saves 4 sregs (s0-s3) vs original's 6 (s0-s5); the
  candidate also uses a3 (original never does) and only t0-t2 (original t0-t5).
  The 2 missing prologue sreg-saves + 4 more instr elsewhere = 6 total.
- This is a deep allocator-version difference: EGC keeps fewer values live
  across the 6+ calls (FlushCache x3, PutDrawBufferLarge, append, SyncPath,
  PutDispBuffer) than the original compiler, so it needs fewer callee-saved regs.

## Final outcome: BLOCKED (2026-09-28), reverted to INCLUDE_ASM
Tried: C reordering, explicit locals, 2/4 pins, scis tied-barrier (expert),
scis-register pins (scis10->t4/scis12->t3). The scis structure + t4/t3 stores
now match.

last-resort GPT-5.6 Sol used. Recommendation (applied): (1) restore the early
loop-index init — `int i = 0;` at the top (before the mid-block calls) and
`for (; i < tiles; i++)` — so `i` is live in a callee-saved reg across the six
calls; (2) make the loadImageBuffer pointer live via a `u8* imageBuffer =
loadImageBuffer;` local used for both FastMemSet and sceGsExecLoadImage. These
moved the function from 0x2e8 to within 4 bytes and revealed the true target
layout: original keeps loadImageBuffer hi in s3 / full in s5 (lui s3,%hi;
move s5,s3; addiu at each use) with i in s4, saving s0-s5.

Residual wall (uncontrollable EGC 2.95.2 allocator artifact): EGC keeps the full
loadImageBuffer pointer in a single sreg (s4) with i in s3, saving s0-s4
(0x2fc, 4 bytes short). Pinning i->s4 overshoots to 0x304 (+2 instr); pinning
imageBuffer->s5 alone stays 0x2fc; -fno-schedule-insns and -fno-schedule-insns2
both make it worse (0x2cc). The 0x300 split-address prologue cannot be landed
with source structure, scoped pins, or per-TU scheduler flags.

Note: SetPalMode__Fi encodes an int param (`__Fi`); the current source declares
it `void SetPalMode(void) asm("SetPalMode__Fi")` (callers pass nothing). A
correct signature would be `void SetPalMode(int)` with callers updated.

Reverted to INCLUDE_ASM (generated .s matches the original byte-for-byte).
Recorded in decomp_state/blocked.json.
