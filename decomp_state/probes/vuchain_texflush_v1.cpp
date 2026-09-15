#include "types.h"

extern volatile u32* volatile vu1ChainHead __attribute__((section(".data")));
extern volatile u32* vu1ChainHeadStore;

#define VU1_DATA_REF_TAG 0x30000000
#define VU1_DATA_REF_END_TAG 0x50000000

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
