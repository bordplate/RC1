#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU0_loadMicroProgram__FPl);

extern int currentVuChain;
extern int vuChainTable[];

int* vuChain_getCurrent(void) {
    int index = *(int*)0x15ED84;
    if (index >= 19) {
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

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_texFlush__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_gsRegsNormal__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233C28);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233C88);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233C90);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233CF0);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", DMAC_VIF1_Enable__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", DMAC_VIF1_Disable__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233E00);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233F00);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", func_00233F78);
