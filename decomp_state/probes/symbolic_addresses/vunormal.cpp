// Negative/generalization test: this source does NOT match under the model.
typedef unsigned int u32;
extern u32* volatile vu1ChainHead;
extern u32 vu1GsRegsNormal[];

void VU1_gsRegsNormal() {
    vu1ChainHead[0] = 0x30000003;
    vu1ChainHead[1] = (u32)vu1GsRegsNormal;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = 0x50000003;
    vu1ChainHead = vu1ChainHead + 4;
}
