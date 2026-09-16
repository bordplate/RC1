#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU0_loadMicroProgram__FPl);

extern int currentVuChain;
// Index (0..18) of the currently selected VU chain in vuChainTable.
extern int currentVuChainIndex;
extern int vuChainTable[];

// Number of entries in vuChainTable.
#define VU_CHAIN_TABLE_SIZE 19

int* vuChain_getCurrent(void) {
    int index = currentVuChainIndex;
    if (index >= VU_CHAIN_TABLE_SIZE) {
        index = 0;
    }
    int* chain = vuChainTable + index;
    currentVuChain = *chain;
    return chain;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_initChain__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_swapChain__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_sendChain__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_syncChain__Fi);

// Head of the VU1 command chain; draw functions append packets through it too.
// Double volatile: EGC must re-read the head before each packet store; a plain
// or single-volatile declaration common-subexpression-eliminates the later
// loads into one shared base register.
extern volatile u32* volatile vu1ChainHead __attribute__((section(".data")));
// Same object as vu1ChainHead, declared plain so the head update is emitted as
// a GP-relative store, matching the original.
extern volatile u32* vu1ChainHeadStore;

// VIF packet tag for a VU1 data-reference record (VIF code 0x30).
#define VU1_DATA_REF_TAG 0x30000000
// VIF end-of-record tag for a VU1 data-reference packet (VIF code 0x50).
#define VU1_DATA_REF_END_TAG 0x50000000

// 12-word GS register state block loaded into the VU1 stream by
// VU1_gsRegsNormal.
extern u32 vu1GsRegsNormal[];

// 44-word GS state block streamed at the end of the font draw chain by
// VU1_gsRegsFont (the only caller is the font VU1 builder in draw.cpp,
// func_001F76A0). Like the gsRegs blocks above it opens with the 0x00008001
// header and carries GS state commands (SETVJUMPC/SETVKEYR-G-B among them).
extern u32 vu1GsRegsFont[];

void VU1_addDataRef(void* dataRef, s32 tag) {
    vu1ChainHead[0] = VU1_DATA_REF_TAG | tag;
    vu1ChainHead[1] = (u32)dataRef;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = 0;
    vu1ChainHeadStore = vu1ChainHead + 4;
}

// Dead tail of VU1_addDataRef: EGC emits this store after the `jr $ra` of the
// matched function above (unreachable); no source form regenerates it without
// changing the matched body, so the orphan is retained.
INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233880);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233888);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233930);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233938);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_addGSregister__FUiUlb);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233A38);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_setScissor__Fiiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233B60);

// Append a VU1 packet that streams the texture-flush GS state block
// (vu1GsRegsTexFlush, 0x1DEE00) to the VU1: [tag, block address, 0, end tag],
// then advance the chain head.
// Codegen constraint: as in VU1_gsRegsNormal, the block address must stay a
// two-instruction RTL pair (0x1E0000 split page, then signed 0x1200 low part)
// interleaved around the first store, pinned with zero-byte asm barriers.
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

// Append a VU1 packet that streams the normal GS register state block
// (vu1GsRegsNormal, 0x1DE3C0) to the VU1: [tag, block address, 0, end tag],
// then advance the chain head.
// Codegen constraint: the original interleaves the block address as two
// instructions (0x1E0000 split page, then signed 0x1C40 low part) around the
// first store. EGC folds a plain constant to an unsigned split, and a
// vu1GsRegsNormal reference becomes one unsplittable `la` under this TU's
// -mno-split-addresses. The zero-byte asm barriers pin the RTL order (head
// load, tag, address high) and keep the address as a two-instruction RTL pair,
// reproducing the original byte for byte.
void VU1_gsRegsNormal() {
    volatile u32* packet = vu1ChainHead;
    asm volatile("" : : "r"(packet));
    u32 tag = VU1_DATA_REF_TAG | 3;
    asm volatile("" : : "r"(tag));
    u32 address = 0x001E0000;
    asm volatile("" : "+r"(address) : "r"(packet), "r"(tag));
    packet[0] = tag;
    address -= 0x1C40;
    vu1ChainHead[1] = address;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 3;
    vu1ChainHeadStore = vu1ChainHead + 4;
}

// Variant of the VU1 GS register state block (0x1DE3F0, 12 words; differs
// from vu1GsRegsNormal only in word 4) streamed by VU1_gsRegsAlt.
extern u32 vu1GsRegsAlt[];

// 12-word GS state block streamed by VU1_texFlush, which the VU1 draw
// pipeline appends after texture work to reset texture state.
extern u32 vu1GsRegsTexFlush[];

// Append a VU1 packet that streams the alternate GS register state block
// (vu1GsRegsAlt, 0x1DE3F0) to the VU1: [tag, block address, 0, end tag],
// then advance the chain head. The draw pipeline sends this before draw
// batches and follows some with VU1_gsRegsNormal to restore the normal state.
// Codegen constraint: as in VU1_gsRegsNormal, the block address must stay a
// two-instruction RTL pair (0x1E0000 split page, then signed 0x1C10 low part)
// interleaved around the first store, pinned with zero-byte asm barriers.
void VU1_gsRegsAlt() {
    volatile u32* packet = vu1ChainHead;
    asm volatile("" : : "r"(packet));
    u32 tag = VU1_DATA_REF_TAG | 3;
    asm volatile("" : : "r"(tag));
    u32 address = 0x001E0000;
    asm volatile("" : "+r"(address) : "r"(packet), "r"(tag));
    packet[0] = tag;
    address -= 0x1C10;
    vu1ChainHead[1] = address;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 3;
    volatile u32* newHead = vu1ChainHead + 4;
    vu1ChainHeadStore = newHead;
}

// Dead tail of VU1_gsRegsAlt: EGC emits this store after the `jr $ra` of the
// matched function above (unreachable); no source form regenerates it without
// changing the matched body (a return-value form fires it but reallocates the
// body registers), so the orphan is retained.
INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233C88);

// Append the font-pipeline GS state block (vu1GsRegsFont, 0x13CF10) to the
// VU1 chain: [tag, block address, 0, end tag], then advance the head.
// Codegen constraint: as in VU1_gsRegsNormal, the block address must stay a
// two-instruction RTL pair (0x140000 split page, then signed 0x30F0 low
// part) interleaved around the first store, pinned with zero-byte asm
// barriers. The record tags carry the 0xB low byte here (vs 3 in the
// gsRegsNormal/Alt pair).
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

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233CF0);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", DMAC_VIF1_Enable__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", DMAC_VIF1_Disable__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233E00);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233F00);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233F78);
