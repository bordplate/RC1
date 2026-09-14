# VU1_gsRegsFont__Fv (0x233C90, 0x5C) — MATCHED 2026-09-14

Appends the VU1 packet that streams the font-pipeline GS state block to the
VU1: `[0x3000000B, 0x13CF10, 0, 0x5000000B]` at the chain head, then advances
`vu1ChainHeadStore` to head+4. Data block `vu1GsRegsFont` (0x13CF10, 0xB0
bytes, renamed from D_0013CF10) named in config/symbols.txt.

Structural twin of the matched gsRegs pair (VU1_gsRegsNormal__Fv 0x233BC8,
VU1_gsRegsAlt__Fv 0x233C28): same five self-based `vu1ChainHead` loads, same
packet shape, same GPREL delay-slot head store. Two immediate differences:
both record tags carry the 0xB low byte (0x3000000B / 0x5000000B) instead of
3, and the block address is 0x13CF10 (signed split `lui 0x14; addiu -0x30F0`,
lo16 0xCF10 >= 0x8000).

The 0x13CF10 block opens with the same 0x00008001 header word as the
gsRegsNormal/Alt blocks and carries GS state command records (0x4C
SETVJUMPC, 0x4D SETVKEYR, 0x4E SETVKEYG, 0x4F SETVKEYB among them) — vertex
shader state, not the raster-state mix of the normal/alt blocks.

## Caller

Single caller: 0x1F7868 inside func_001F76A0 (draw.cpp, the VU1 font-window
builder in the font API section next to FontPrintWindow 0x1F7090 /
FontSetWindow 0x1F7668). func_001F76A0 builds a large VU1 packet (VU1
programs at 0x186F80/0x187000, float data from 0x18CE90-0x18CF2C, one
data-ref to 0x10E810 via VU1_addDataRef) and ends by appending this GS state
block. func_001F76A0's caller func_001F79A8 is reached from
DrawDebugProfiler (0x1F39D0), which also calls the gsRegsNormal/Alt pair.

No dead-tail fragment follows this function: the final GPREL head store sits
in the `jr` delay slot (alive) and the next function starts 8 bytes on.

## Solution

`code/game/vuchain.cpp`, TU flag `-mno-split-addresses` (vuchain.o). The
barrier-pinned address RTL is identical to VU1_gsRegsNormal's (see that note
for the mechanism); only the split page/low part and the tag low byte change:
0x13CF10 = 0x140000 - 0x30F0, so `address -= 0x30F0;` gives the original's
`lui 0x14; addiu -0x30F0` signed split (the unsigned-fold failure mode
applies exactly as there — a raw constant or named-symbol `la` cannot
interleave hi/lo around the first store).

```cpp
void VU1_gsRegsFont() {
    volatile u32* packet = vu1ChainHead;
    asm volatile("" : : "r"(packet));
    u32 tag = VU1_DATA_REF_TAG | 0xB;
    asm volatile("" : : "r"(tag));
    u32 address = 0x00140000;
    asm volatile("" : "+r"(address) : "r"(packet), "r"(tag));
    packet[0] = tag;
    address -= 0x30F0;
    vu1ChainHead[1] = address;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 0xB;
    vu1ChainHeadStore = vu1ChainHead + 4;
}
```

Matched byte-for-byte on the first build (24 words at 0x233C90 identical;
clean `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes).
