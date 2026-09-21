# LoadPifAsPSMT8H (0x1E9168, size 0x1D0, 116 words)

`code/game/bloaders.cpp`. `extern "C" void LoadPifAsPSMT8H(u8* source, u8* destination,
int offset, int size)`; a0=source (PifHeader*), a1=destination (u64* gsRegs), a2=offset,
a3=size. PSM T8H debug-font PIF loader, called by `LoadDebugFont`.

## Role
Zero-clears a 0x54-byte `PifParser pp` at sp+0x0 (FastMemSet), sets pp fields from the
`PifHeader` at source, builds a 96-byte GS load packet (`sceGsLoadImage image`) at
sp+0x60, runs two `(sceGsSetDefLoadImage / FlushCache / sceGsExecLoadImage /
sceGsSyncPath)` sequences, then writes a 16-byte PIF header to destination.

- `PifHeader` (0x20): pifID, fileSize, uSize(+0x8), vSize(+0xC), texFormat,
  clutFormat(+0x14), clutOrder, mipLevels; data after header at +0x20.
- `PifParser` (0x54): pClut+0x0, pTex[4]+0x4, cSize+0x14, tSize[4]+0x18, cPos+0x28,
  tPos[4]+0x2C, tbw[4]+0x3C, uLog+0x4C, vLog+0x50.
- Callees (all return `int`): log2dim=0x1F97A0 (`0x1E - clz(x)`, fastfunc.s),
  sceGsSetDefLoadImage=0x122330, FlushCache=0x118A80, sceGsExecLoadImage=0x122658,
  sceGsSyncPath=0x120558.

## Near match (body byte-for-byte)
A C candidate (see git history of this file) reproduces the ENTIRE body from the first
`jal log2dim` onward, and the frame (0x120, 5 s-regs):
- both `jal log2dim` with `sra pal,size,8` in call-1's delay slot and
  `sw v0,pp.uLog` in call-2's delay slot;
- `addiu size-reg,sp,96` REUSING the size register as the image base;
- the dead `mult uSize,vSize` that stores uSize (first operand, not the product) to
  pp.tSize[0];
- the two GS load sequences with identical a0-a3/t0-t3 argument setup;
- the 64-bit PIF-header OR chain (dsll32 for palShift<<37 / clutFormat<<51 / 1<<63,
  dsll for tbw<<14 / uLog<<26 / vLog<<30, lui 0x1b0 for 0x01B00000, li 0x8000 +
  dsll 0x13 for 1<<34) and the three trailing `sd` stores.

## Blocker: prologue sq/move order (s0 vs s1)
The ONLY difference is the prologue. The original emits the parameter sq/move sequence
with s0 before s1:

```
sq s4,256 / sq s2,224 / move s4,a2 / sq s0,192 / move s2,a1 / move s0,a0 /
sq s1,208 / sq ra,272 / move s1,a3 / sq s3,240 / move a0,sp / move a1,zero
```

EGC 2.95.2 emits s1 before s0:

```
sq s4,256 / sq s2,224 / move s4,a2 / sq s1,208 / move s2,a1 / sq s0,192 /
move s1,a3 / sq ra,272 / move s0,a0 / sq s3,240 / move a0,sp / move a1,zero
```

The natural (unpinned) C form also mis-assigns the s-registers (offset->s0, source->s1,
size->s2, dest->s3, palShift->s4 instead of the original source->s0, size->s1, dest->s2,
palShift->s3, offset->s4).

## Variants tested (all fail to match the prologue)
1. Callee `int` return types (sceGsSetDef/Exec/SyncPath) — no effect on the prologue.
2. Typed formals `PifHeader* pPif, u64* gsRegs` (aliases removed) — no effect.
3. Explicit `register asm("$16".."$20")` pinning + zero-byte `asm volatile` constraint —
   fixes the s-register ASSIGNMENT (s0=source, s1=size, s2=dest, s3=palShift,
   s4=offset) but the prologue still emits s1 before s0.
4. `-fno-schedule-insns` — no effect on the prologue order.
5. Pinned-declaration reordering (both s0/s1 orders) — no effect.

The prologue sq/move ordering is an EGC 2.95.2 register-allocation/scheduler heuristic
not controllable from C source. Blocked 2026-09-21; last-resort GPT-5.6 Sol invoked
(recommended the callee-return-type, typed-formal, and register-pinning forms, all
mechanically tested). Retain the generated assembly.
