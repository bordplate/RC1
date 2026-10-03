# PutDrawBufferLarge__Fv (0x001FB2D0, 148 bytes) — matched 2026-10-04

## Semantics

VU1-chain appender for the large draw environment, sibling of the
vuchain/PutDrawBuffer* packet family (shared head 0x160F00):

- If `vu1ChainHead` is set (a VU1 command chain is running), append a
  16-byte VIF data-reference record at the head:
  `[VU1_DATA_REF_TAG | 9, (u32)&aaBuffPtr->giftagDrawLarge & 0xFFFFFFF,
  0, VU1_DATA_REF_END_TAG | 9]`, then advance the head by 4 words.
- Otherwise upload the draw env directly: `sceGsPutDrawEnv(&aaBuffPtr->giftagDrawLarge)`.

`aaBuffPtr` (0x15EEB8) is a pointer to the shared frame-buffer parameter
block; `SetupFS_AA_buffer` installs `&occlCamParamBase` (0x151780) into it
(its 0xFB9A0 stores the occlCamParamBase address through the pointer), and
the buffer-setup/append functions read the GS environment blocks off it.
The giftag block head at struct offset +0x30 is `OcclCamParamBlock::
giftagDrawLarge` (newly named field; SetupFS_AA_buffer writes the two
qwords at +0x30/+0x38 when it fills the block).

The callee `sceGsPutDrawEnv` is core.text SDK code at 0x1221B0 (name
verified against the Lombyte SCUS_971.99 decompilation symbol table).
Ghidra: spins while the GIF DMA channel reports busy (CHCR bit 0x100),
then programs MADR (high bits of the block's first qword) and QWC-1 (low
15 bits) and kicks the transfer with CHCR = 0x101; returns 0 on success,
-1 after a wait timeout. Declared in code/include/sce_gs.h, aliased in
config/linker_aliases.ld.

## Codegen notes (default flags, framebuf.o)

- The `& 0xFFFFFFF` mask on the giftag address (kept from the original
  source; confirmed in the Deadlocked pseudo-code of the same function)
  is what makes EGC emit the original's zero-extend sequence. A bare
  `(u32)` pointer cast or `(u32)((u64)p + 0x30)` emits no AND at all (or a
  dsll32/dsra32 pair); with the mask, EGC materializes
  `lui 0xFFFF; ori 0xFFFF`, which overflows to 0xFFFFFFFF, and the AND
  against it becomes `and r, r, -1` — exactly the original. The mask is a
  documented codegen constant (AA_GIFTAG_ADDR_MASK), no refactor entry.
- The original tests the head and reuses the test's a0 value for the
  first store (`head[0] = ...`), then reloads the volatile head for the
  remaining three stores and the head+4 store-back. Reproduced with a
  plain local: `volatile u32* head = vu1ChainHead;` used for the test and
  the first store. Without the local, EGC emits a fresh volatile reload
  for the first store (144 bytes, 29 diffs). The local must be declared
  `volatile u32*` — cfront rejects a plain `u32*` from the
  double-volatile global (discards qualifiers).
- The final `vu1ChainHeadStore = vu1ChainHead + 4;` lands in the branch
  delay slot; through the plain (non-.data) same-address alias ps2eeas
  emits it as the single GPREL16 access (`sw v0, -0x5D00($28)`) of the
  original — same rule as the whole vuchain family.
- The `b` at 0x340 targets the epilogue (0x358), not the call: the
  `sceGsPutDrawEnv` call happens only in the `beqz a0` (chain inactive)
  path. Earlier misread as call-on-both-paths.
- The struct field form `&aaBuffPtr->giftagDrawLarge` and the named tag
  macros generate byte-identical code to the `aaBuffPtr + 0x30` /
  literal form.

## Verification

- `tools/decomp_probe.py` on the final source form: 148/148 bytes, 0
  differences.
- Full `make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf` pass.
