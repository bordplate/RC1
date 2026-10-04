# PutDrawBufferSmall__Fv (0x001FB3D0, 108 bytes) — matched 2026-10-04

## Semantics

Branchless sibling of the matched PutDrawBufferLarge__Fv in the same TU:
appends the small draw-env VIF data-reference record to the VU1 command
chain unconditionally (no `if (vu1ChainHead)` guard, no sceGsPutDrawEnv
fallback): word 0 = 0x30000009 (data-ref tag, qcnt 9), word 1 =
(u32)&aaBuffPtr->giftagDrawSmall & 0xFFFFFFF, word 2 = 0, word 3 =
0x50000009 (end tag, qcnt 9), then advances the head 4 words through the
plain vu1ChainHeadStore alias (GPREL in the `jr` delay slot).

The small draw-env block is the 9-unit (0x90-byte) GIF record at
OcclCamParamBlock+0xC0 (newly named giftagDrawSmall); SetupFS_AA_buffer
builds it with the same giftag idiom as the large block (+0x30) and sets up
the small draw env via sceGsSetDefDrawEnv at +0xD0. Callers push the chain
first and call this between PutDrawBufferLarge and appendClearBlackDataRef
(startlevel, FadeToBlack and siblings in draw_post_post, SetPalMode,
Transition_DoTransition, DoSpaceTransition, space/func_00231BD8).

## Matching form

Identical to the matched PutDrawBufferLarge__Fv / appendClearBlackDataRef
family in framebuf.cpp (default flags, double-volatile plain
`vu1ChainHead`, `volatile u32* head` local for the first store,
AA_GIFTAG_ADDR_MASK for the zero-extend `and r,r,-1`, final store through
the plain same-address alias vu1ChainHeadStore for the GPREL `jr`-delay
slot):

```cpp
void PutDrawBufferSmall() {
    volatile u32* head = vu1ChainHead;
    head[0] = VU1_DATA_REF_TAG | DRAW_ENV_SMALL_QCNT;
    vu1ChainHead[1] = (u32)&aaBuffPtr->giftagDrawSmall & AA_GIFTAG_ADDR_MASK;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | DRAW_ENV_SMALL_QCNT;
    vu1ChainHeadStore = vu1ChainHead + 4;
}
```

The five head loads are one per store, self-based absolute, exactly as the
original emits them.

Newly named: OcclCamParamBlock::giftagDrawSmall (+0xC0, u64 — the 128-bit
giftag head, second qword inside the pad, same convention as
giftagDrawLarge), DRAW_ENV_SMALL_QCNT 9.

## Pitfall hit this session

The first camera.h edit split pad_038[0x118] as pad_038[0x88] + field +
pad_0C8[0x68] — 0x20 short, shifting drawW/drawH and 10 single-byte offset
diffs in the already-matched level .text (0x152->0x132 etc.). Correct
split: pad_0C8[0x88] (0xC8 + 0x88 = 0x150). Always re-verify a pad split
arithmetically; the full-ELF cmp caught it.

## Verification

- tools/decomp_probe.py on the final source form: 108/108 bytes, 0
  differences.
- Full `make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf` pass.
