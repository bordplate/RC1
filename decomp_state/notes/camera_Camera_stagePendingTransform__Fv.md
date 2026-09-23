# Camera_stagePendingTransform__Fv (func_001EC7F0, vram 0x1EC7F0, file 0xED770, 116 bytes) - MATCHED 2026-09-23

## Behavior

Stages the pending camera transform before a mode switch (see
`Camera_commitPendingTransform` for the commit side):

```cpp
if (camTransState.type != 0)
    return;
camTransState.pendingCam0 = camTransState.activeCam0;      // 16-byte quad
if (camTransState.reqType == 2)
    FastVecAdd(&pendingCam0, &camPosOffset, &pendingCam0); // + position offset
camTransState.pendingCam1 = camTransState.activeCam1;      // 16-byte quad
```

- `camTransState` (0x1871B0) is `currentCamera.blender` (`CamBlender`):
  `type` +0x02 (staged flag, checked by the caller and the commit),
  `reqType` +0x03, active quads +0x50/+0x60, pending quads +0xC0/+0xD0.
- Called from `func_001EC8A0` at 0x1EC9E4 (when `reqType == 2`, followed by
  `func_001EC710`) and 0x1ECA04 (when `reqType != 0`, followed by a
  pendingCam1 -> +0xB0 copy).
- `camPosOffset` (0x13F490, .data): 16-byte camera position offset, zero in
  boot (level-provided). Second user at 0x1ECB78 adds the same quad to the
  camera-slot quad at +0x30, confirming the shared-offset semantics.
- `FastVecAdd` (0x1F9A10): VU1 `lqc2/vadd.xyz/sqc2` quad add, sibling of
  `FastVecSub` (0x1F9A28) in game/fastfunc; C linkage.

## Matching form

```cpp
CameraQuad* dst0 = &camTransState.pendingCam0;
asm volatile("" : "+r"(dst0));
register CameraQuad* src0 asm("$3") = &camTransState.activeCam0;
asm volatile("" : "+r"(src0));
register CameraQuad val asm("$2") = *src0;
*dst0 = val;
register int reqType asm("$4") = camTransState.reqType;
asm volatile("" : "+r"(reqType));
register int two asm("$2") = 2;
if (reqType == two)
    FastVecAdd((void*)dst0, (void*)&camPosOffset, (void*)dst0);
```

Target shape: copy1 `addiu a1,s0,192; addiu v1,s0,80; lq v0,0(v1);
sq v0,0(a1)`, then `lbu a0,3(s0); li v0,2; bne a0,v0`, 4-instruction call
setup `move a0,a1; lui a1,0x14; move a2,a0; jal; addiu a1,a1,-2928`
(a2 from a0 because a1 is overwritten by the constant), then the
pendingCam1 copy and a standard 0x20-frame epilogue.

- Unpinned base `camTransState` (a natural `.data` member expression) lands
  in s0 with the original prologue (`lui v0,0x18; addiu s0,v0,29104`);
  pinning the base flips it to t8/self-based forms.
- src0 pinned to $3 keeps dst0 in a1 and val in v0; val pinned to $2
  together with src0 is the copy1 shape. Pinned dst0 ($5) breaks the call
  setup (5 instructions, constant via v0, `move a2,a1` in the jal delay
  slot), so dst0 must stay unpinned with a tied `"+r"` barrier.
- The comparison cluster: EGC's natural allocation hoists `li` into the
  lq/sq gap and puts the `lbu` in v1 (src0's dead home). Pinning reqType to
  $4 (as an `int`, NOT `u8` — a sub-word register local is silently
  unpinned) gives `lbu a0`; pinning the constant to $2 with a live range
  DISJOINT from val's (val dies at the sq, `two` is born after it) lets EGC
  reuse v0 for both in sequence — `li v0,2` after the sq, as in the
  original. A tied `"+r"` barrier between the lbu and the li statement
  forces the lbu-ahead-of-li order.

## Failed variants

- Input-only `asm volatile("" : : "r"(reqType))` between lbu and li: does
  NOT enforce the order in this EGC and moves the base to a2 (frame 0x10).
- Big noreorder inline-asm hybrid (C prologue/copy1 + asm test/call/tail):
  2-diff near miss — the post-asm C tail `*dst1 = *src1` allocates its 128-bit
  value to s0 (the dead base's home) instead of v0. Any $2-pinned value live
  after the asm, or any C use of the asm outputs, flips the base to t8.
- `volatile CameraQuad*` pointer form: DImode offset folding
  (`lq v0,80(s0); sq v0,192(s0)`).
- Typed pointer arithmetic `blend + 0xC0`: scaled by sizeof(CamBlender).
