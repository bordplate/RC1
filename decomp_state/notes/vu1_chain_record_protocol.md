# VU1 chain record protocol (VIF tag codes and qcnt)

Established 2026-10-04 while resolving the `vuchain_dataref_record_ids`
refactor entry. Explains the low byte of every record tag in the VU1 command
chain appenders (the former "record ID" literals).

## Tag layout

Every chain record starts with a 16-byte tag block. Word 0 is
`CODE | qcnt` where `qcnt` (the low byte) is the **payload size in 16-byte
units**. The record occupies the tag block plus that many payload blocks in
the chain; word 3 of the tag block is the end tag `0x50000000 | qcnt` with
the same qcnt.

| code | meaning |
|------|---------|
| 0x30 | data reference: word 1 holds the main-memory address of the payload; the VIF streams the qcnt blocks from there into VU1 memory later |
| 0x10 | direct data: word 1 is 0 and the qcnt payload blocks are appended inline after the tag block (the 0x233888 appender also stores `0x1000404` in word 2 and `addr | (qcnt << 16) | 0x6C000000` in word 3) |
| 0x90 | VIF1 command (Vif1ChainCmd): word 0 = cmd + 0x90000000, rest zero |

The VU1_addGSregister / VU1_setScissor family opens a 0x10-code record with
qcnt 2 (`[0x10000002, 0, 0, 0x50000002]` + GIFTAG `[0x00008001, 0x10000000,
0x0E, 0]`) and appends 8-byte register entries inline.

## Payload blocks are pre-composed GS GIF streams

The streamed blocks (vu1GsRegs*, resetGsRegsFixed, gsStateFade*,
aaClearBlackRegs, aaDisplayRegs, the draw-env record) are sequences of GS
GIFTAG records. GIFTAG word 1's top byte is the number of following 16-byte
data blocks (packs), which is consistent with the qcnt of every static block
below.

## Verification (boot ELF data layout)

qcnt == payload bytes / 16 for every statically sized block:

| qcnt | payload (bytes, words)  | extent               |
|------|-------------------------|----------------------|
| 3    | vu1GsRegsNormal (48, 12) | 0x1DE3C0..0x1DE3F0 |
| 3    | vu1GsRegsAlt (48, 12)    | 0x1DE3F0..0x1DE420 |
| 3    | vu1GsRegsTexFlush (48,12)| 0x1DEE00..0x1DEE30 |
| 0xB  | vu1GsRegsFont (176, 44)  | 0x13CF10..0x13CFC0 |
| 0x13 | resetGsRegsFixed (304,76)| 0x13CFC0..0x13D0F0 |
| 0x14 | gsStateFade (320, 80)    | 0x13CDD0..0x13CF10 |
| 0x14 | gsStateFadeColor (320,80)| 0x13CC90..0x13CDD0 |
| 9    | large draw-env record (144, 36), runtime block at OcclCamParamBlock+0x30 built by SetupFS_AA_buffer |
| 0x15 | aaClearBlackRegs (runtime, framebuf_appendLargeSetup) |
| 0x29 | aaDisplayRegs (runtime, framebuf_appendSmallSetup) |

## Deadlocked corroboration

reference/dl/game_dl/vuchain.cpp (same engine) shows the original idiom:
`VU1_addDataRef(void *data, int qcnt)` stores `qcnt | 0x30000000`;
`VU1_addDataDirect` copies `qcnt * 0x10` payload bytes and advances the head
by qcnt; the font texture path builds tags as `iVar8 >> 4 | 0x30000000`
(bytes >> 4 = /16). The RC1 binary matches this protocol exactly (0x233888
is the RC1 VU1_addDataDirect; VU1_addGSregister 0x233980 matches the
Deadlocked record layout word for word).

## Source form (post-refactor)

- Static payloads: arrays are declared with their exact word counts and the
  tag low byte is derived as `sizeof(block) / 16` (vuchain.cpp,
  draw_post_post.cpp).
- Runtime payloads: named qcnt constants — `DRAW_ENV_LARGE_QCNT 9`
  (framebuf.cpp); `VU1_addDataRef(void*, s32 qcnt)` parameter renamed from
  `tag` to the original's `qcnt` name.
- The 0x10/0x50 0x0E and 0x10/0x50 0x06 inline-data records and the 0x15/0x29
  records live only in nonmatching INCLUDE_ASM (func_001FB440,
  func_001FB8F0, framebuf_appendLargeSetup, framebuf_appendSmallSetup); they
  keep raw constants until those functions are decompiled.
- The 4-word record stride (`vu1ChainHead + 4`) stays as-is per the
  STYLEGUIDE buffer-size exception.
