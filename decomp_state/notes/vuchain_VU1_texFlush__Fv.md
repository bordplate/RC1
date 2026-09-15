# VU1_texFlush__Fv (0x233B68, 0x5C) — MATCHED 2026-09-15

Appends the VU1 packet that streams the texture-flush GS state block to the
VU1: `[0x30000003, 0x1DEE00, 0, 0x50000003]` at the chain head, then advances
`vu1ChainHeadStore` to head+4. Data block `vu1GsRegsTexFlush` (0x1DEE00,
12 words, renamed from D_001DEE00) named in config/symbols.txt.

Structural twin of the matched gsRegs trio (VU1_gsRegsNormal__Fv 0x233BC8,
VU1_gsRegsAlt__Fv 0x233C28, VU1_gsRegsFont__Fv 0x233C90): same five
self-based `vu1ChainHead` loads, same packet shape, same GPREL delay-slot
head store. In fact the original bytes are identical to VU1_gsRegsNormal's
0x5C except ONE word: the address low part, `addiu v1, -0x1200` here vs
`addiu v1, -0x1C40` there (0x1DEE00 = 0x1E0000 - 0x1200 vs
0x1DE3C0 = 0x1E0000 - 0x1C40). The 0x1DEE00 block opens with the same
0x00008001 header word as the gsRegsNormal/Alt blocks.

## Callers

Seven, all texture/geometry draw builders:
SetupGifPaging__Fi (draw, call at 0x1F4414), func_0020CCA8 (mobyfunc,
0x20CE74), func_00228A30 (shrubfunc, 0x228978), func_0022B688 (skyfunc,
0x22B5D4), DrawTfrag (tfragfunc, 0x233250), DmaTieTextures__Fv (tiefunc,
0x2356C8), func_00239D50 (vendor, 0x239C04). DrawTfrag's call sits right
after its texture DMA/upload steps, consistent with a post-texture-work
state reset.

No dead-tail fragment follows this function: the final GPREL head store sits
in the `jr` delay slot (alive) and the next function (VU1_gsRegsNormal)
starts 8 bytes on.

## Solution

`code/game/vuchain.cpp`, TU flag `-mno-split-addresses` (vuchain.o). The
barrier-pinned address RTL is identical to VU1_gsRegsNormal's (see that note
for the mechanism); only the low part changes: `address -= 0x1200;` gives
the original's `lui 0x1E; addiu -0x1200` signed split (the unsigned-fold
failure mode applies exactly as there — a raw constant or named-symbol `la`
cannot interleave hi/lo around the first store).

```cpp
void VU1_texFlush() {
    volatile u32* packet = vu1ChainHead;
    asm volatile("" : : "r"(packet));
    u32 tag = VU1_DATA_REF_TAG | 3;
    asm volatile("" : : "r"(tag));
    u32 address = 0x001E0000;
    asm volatile("" : "+r"(address) : "r"(packet), "r"(tag));
    packet[0] = tag;
    address -= 0x1200;
    vu1ChainHead[1] = address;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 3;
    vu1ChainHeadStore = vu1ChainHead + 4;
}
```

Verified with decomp_probe.py (vuchain_texflush_v1, 92/92 bytes,
--flags=-mno-split-addresses, --define vu1ChainHeadStore=0x160f00), then
byte-for-byte on the first production build (23 words at 0x233B68 identical;
clean `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes).
