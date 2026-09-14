#include "types.h"

extern volatile u32* volatile vu1ChainHead __attribute__((section(".data")));
extern volatile u32* vu1ChainHeadStore;
extern u32 vu1GsRegsNormal[];

#define VU1_DATA_REF_TAG 0x30000000
#define VU1_DATA_REF_END_TAG 0x50000000

void VU1_gsRegsNormal() {
    vu1ChainHead[0] = VU1_DATA_REF_TAG | 3;
    vu1ChainHead[1] = (u32)vu1GsRegsNormal;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 3;
    vu1ChainHeadStore = vu1ChainHead + 4;
}
