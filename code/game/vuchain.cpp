#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU0_loadMicroProgram__FPl);

extern int currentVuChain;
extern int vuChainTable[];

int* vuChain_getCurrent(void) {
    int idx = *(int *)0x15ED84;
    if (idx >= 19) idx = 0;
    int* p = vuChainTable + idx;
    currentVuChain = *p;
    return p;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_initChain__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_swapChain__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_sendChain__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_syncChain__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/vuchain", VU1_addDataRef__FPvi);

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
