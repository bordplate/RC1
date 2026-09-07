#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_GetName);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", func_00209168);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_RestoreGame);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", func_00209370);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_Update);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_MakeWholeSave);

extern "C" int func_001233F0(void);
extern char D_001E8360[];
extern "C" void STUB_printf(const char* fmt, ...);

extern "C" void memcard_Init(void) {
    if (func_001233F0() != 0)
        STUB_printf(D_001E8360);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_GetDataSize);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_Checksum);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_TestChecksum);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_PrepData);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_RestoreInfo__FPcii);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_RestoreData__FPcT0iP7mc_data);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_Save__Fii);
