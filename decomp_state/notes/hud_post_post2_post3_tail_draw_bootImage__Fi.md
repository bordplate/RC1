# draw_bootImage__Fi (0x2012B8, 0x264 = 612 bytes) — BLOCKED (2026-10-12)

Boot-splash **per-row VU1 blit-packet** chain builder. `draw_bootImage(int
image)` reads `videoModePal` (width = PAL 0x1C0 / NTSC 0x1A0) and `zbufBase`
(>>8 = running z offset), then loops (do-while) appending one 0x80-byte VU1
packet per row and advancing `*vu1ChainHead` by 0x80 each row; after the loop it
appends one 0x30-byte end packet. Blocked on EGC 2.95.2 default-scheduler
loop-invariant-hoisting + store-order tie-breaks after the CFG and every packet
field were SOLVED (do-while correction from last-resort GPT-5.6 Sol; the
high-32-bit z/stride words and the stride<<11 image advance were read off the
original objdump). Best candidate (v4) = 616 bytes / 139 diff words; the whole
loop body's *values* are byte-correct, only the prologue interleaving, 4-constant
hoisting, and one store-order position differ.

## What it does (verified against `objdump assets/boot_elf.elf`)

`width`/`rem`/`stride` are **s64**; `zbuf` and `image` are int. The loop is a
**do-while** (no entry test; back-edge `bgtz width` at 0x20146c). Per row, with
H = `*vu1ChainHead`:

- header (4 sw through *vu1ChainHead): H+0x00 = 0x10000006, H+0x04 = 0,
  H+0x08 = 0, H+0x0C = 0x50000006; then *vu1ChainHead = H+0x10.
- 12 64-bit sd's (first base H, rest base H+0x10), ORIGINAL order:
  H+0x10 = 0x4000000000000001 (W4); H+0x20 = ((u64)(u32)zbuf << 32) |
  0x0008000000000000; H+0x40 = ((u64)stride << 32) | 0x200; H+0x60 =
  ((u64)stride << 7) | 0x0800000000008000 (W14); H+0x68 = 0; H+0x18 =
  0x0EEEEEEE (SEVEN E's: `lui s7,0xeee; ori s7,0xeeee`); H+0x28 = 0x50; H+0x30 =
  0; H+0x38 = 0x51; H+0x48 = 0x52; H+0x50 = 0; H+0x58 = 0x53.
- zbuf += (int)(stride << 3)  [loop-carried accumulator]
- *vu1ChainHead = H+0x70; H+0x70 = (u32)((stride << 7) | 0x30000000) [VIF ref
  tag]; H+0x74 = image; image += (int)(stride << 11); H+0x78 = 0; H+0x7C =
  (u32)(stride << 7) | 0x50000000 [VIF ref end tag]; *vu1ChainHead = H+0x80
  (GPREL sw in the `bgtz` delay slot).
- stride = (rem >= 0) ? width : 0x80  [emitted `slt v0,-1,rem; movz`; NOT `rem<0`]
- width = rem.

End packet (H' = *vu1ChainHead after the loop): H'+0x00 = 0x10000002, H'+0x04 =
0, H'+0x08 = 0, H'+0x0C = 0x50000002; *vu1ChainHead = H'+0x10; H'+0x10 =
0x1000000000008001; H'+0x18 = 0xE; H'+0x20 = 0; H'+0x28 = 0x3F; *vu1ChainHead =
H'+0x30. Frame 0x90, saves s0–s8 (the original keeps all 13 packet constants
live in t7,t6,s0,s1,s2,s3,s4,s5,s6,s7,t9,t8).

## The idiom (works, proven by the matched loop-free sibling)

`extern volatile u32* volatile vu1ChainHead;` (double-volatile, no section
attr) in a ps2eeas TU: EGC re-reads the head before every packet store and
ps2eeas expands each bare pseudo self-based (body) / GPREL (delay slot). The
loop-free sibling `OcclAppendRectsHi` in `code/game/hud_post_post2_post3_tail_occl.cpp`
matches byte-for-byte with this idiom + `*(u64*)&head[N]` stores. This function
adds a **loop** and **13 loop-invariant constants**, which is the wall.

## Residual 139 words (v4 = do-while + s64 + corrected fields)

Pure EGC 2.95.2 default-scheduler / RA tie-breaks, NOT value bugs:
1. Prologue: frame-first is reproduced, but EGC interleaves the `sq` saves with
   the constant `li`/`dli` in a different order than the original (which splits
   the two pkt-tag constants' hi/lo across the `sq` saves).
2. EGC hoists 9 of 13 constants to the prologue and re-materializes W4
   (0x4000000000000001), ROW_HI (0x0008000000000000), W14
   (0x0800000000008000) and 0x200 as `dli` inside the loop each iteration
   (register-pressure cost decision); the original keeps all 13 live.
3. Loop-body sd order: EGC places H+0x58 (0x53) in position 5 and H+0x68 (0) in
   position 6; the original has H+0x68 in position 5 and H+0x58 last.
These three coupled scheduling choices leave the 4-byte (616 vs 612) size gap.

## Tried (all parity-safe, none closed it)

- v1: `while` loop, 64-bit #defines, u64* data → 496B / 150.
- v2: corrected the 64-bit store indices (u64* not u32*) → 588B / 141.
- v3: precompute the invariant 64-bit consts into locals → 588B / 141 (no hoisting change).
- v4: **do-while + s64 width/rem/stride + high-32 z/stride words + stride<<11
  image advance** (last-resort corrections) → 616B / 139 (BEST).
- v5: pin W4→$24, ROW_HI→$22, W14→$16, 0x200→$19 + tied `+r` barrier → 624B / 149 (WORSE;
  the pins disrupt EGC's allocation and add saves).
- Flags: default 616/139 (best); -fno-schedule-insns 564/150; -fno-schedule-insns2 588/147.
- Isolated Splat segment (own TU, default SN) was tried for flag isolation; the
  TU default flags are correct (sibling matches with them) so the split is not
  the lever — it was reverted (function restored to `hud_post_post2_post3_tail.cpp:96`).

Retain INCLUDE_ASM; full-ELF parity preserved. Re-attempt start point: v4 (this
note's "What it does" is the verified field table). See blocked.json
`code/game/hud_post_post2_post3_tail.cpp:draw_bootImage__Fi`.
