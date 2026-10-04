# appendClearBlackDataRef (framebuf.cpp, 0x1FB368)

Matched 2026-10-04. 26 words (0x68). Object diff clean on first candidate;
full boot ELF cmp byte-identical.

## Semantics

Appends the AA pass's clear-to-black register block to the VU1 command chain
as a VIF data-reference record when the chain is running (vu1ChainHead != 0):
word 0 = 0x30000015 (data-ref tag, qcnt 0x15 = 21 x 16-byte blocks), word 1 =
&aaClearBlackRegs, word 2 = 0, word 3 = 0x50000015 (end tag), then advances
vu1ChainHead by 4 words. No else branch, no frame (no calls).

The payload block (aaClearBlackRegs, 0x152040) is built at runtime by
SetupFS_AA_buffer (0x1FA978, blocked): it composes a GS GIF stream there —
giftag `0x10000001` (16 packs) at 0x152040 plus register payload words — so
the boot-ELF bytes are all zero. qcnt 0x15 is the streamed size per the VU1
chain record protocol (notes/vu1_chain_record_protocol.md).

Callers (all push the chain then call this between PutDrawBufferLarge and
PutDrawBufferSmall): startlevel (bmain), drawNormalFrame, FadeToBlack and
siblings in draw_post_post, SetPalMode, Transition_DefaultDraw, and
space/func_0022F288.

## Deadlocked corroboration

reference/dl/game_dl/framebuf.cpp `ClearDrawBufferLarge(int black)` fast path
is the direct descendant: `(*vu1_bufPtr)[0] = 0x30000015;
piVar2 = black ? aaClearBlackRegs : aaClearRegs; (*vu1_bufPtr)[1] = (int)piVar2;
(*vu1_bufPtr)[2] = 0; (*vu1_bufPtr)[3] = 0x50000015; vu1_bufPtr += ...`.
RC1 split the `black` flag into dedicated functions: this one is the black
variant; the unguarded siblings append aaClearRegs (0x1FB680, qcnt 0x26) and
aaDisplayRegs (0x1FB6E0, qcnt 0x29). DL names the block
`long long int aaClearBlackRegs[21]`.

## Matching form

Identical to the matched sibling PutDrawBufferLarge__Fv in the same TU
(double-volatile plain vu1ChainHead, default flags, no -mno-split-addresses):

```cpp
volatile u32* head = vu1ChainHead;
if (head) {
    head[0] = VU1_DATA_REF_TAG | AA_CLEAR_BLACK_QCNT;
    vu1ChainHead[1] = (u32)aaClearBlackRegs;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | AA_CLEAR_BLACK_QCNT;
    vu1ChainHead = vu1ChainHead + 4;
}
```

- `(u32)aaClearBlackRegs` has no AA_GIFTAG_ADDR_MASK: the original emits the
  plain self-based `lui/addiu` address load with no zero-extend `and`
  (unlike PutDrawBufferLarge's runtime giftag pointer).
- The final store `vu1ChainHead = vu1ChainHead + 4;` stores through the plain
  vu1ChainHead name itself and stays SELF-BASED (`lui at; sw r,off(at)`)
  before `jr $ra` with a nop delay slot — EGC did not schedule it into the
  delay slot here, unlike PutDrawBufferLarge (GPREL via the vu1ChainHeadStore
  alias in a `b` delay slot) and the unguarded small variants (GPREL in the
  `jr` delay slot). No alias needed.
- EGC keeps the first head load in a0/a0-like scratch ($3) for the beqz guard
  and the `head[0]` store; the four subsequent `vu1ChainHead[...]` accesses
  each reload the pointer fresh (double volatile), matching the original's
  five self-based lui/lw pairs.

## Naming

- framebuf_appendLargeSetup__Fv -> appendClearBlackDataRef (symbols.txt
  0x1FB368; C++ declaration mangling gives appendClearBlackDataRef__Fv).
- Callers updated in bmain.cpp and draw_post_post.cpp
  (plain C++ declarations mangle to the new symbol automatically).
- AA_CLEAR_BLACK_QCNT 0x15 named per the record-protocol convention for
  runtime payloads.
- aaClearBlackRegs declared `extern u32 aaClearBlackRegs[AA_CLEAR_BLACK_QCNT * 4]`
  (the streamed extent).

## Verification

- fdiff 0x1FB368 0x68: 0 word diffs of 26 (linked ELF).
- make clean split/build: full `cmp build/boot_elf.elf assets/boot_elf.elf`
  passes.
