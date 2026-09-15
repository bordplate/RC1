// Negative/generalization test: this source does NOT match under the model.
typedef unsigned int u32;
extern u32* volatile vu1ChainHead;

void VU1_addDataRef(void* dataRef, int tag) {
    vu1ChainHead[0] = 0x30000000 | tag;
    vu1ChainHead[1] = (u32)dataRef;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = 0;
    vu1ChainHead = vu1ChainHead + 4;
}
