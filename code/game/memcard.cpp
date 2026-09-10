#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_GetName);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", func_00209168);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_RestoreGame);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", func_00209370);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_Update);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_MakeWholeSave);

extern "C" int func_001233F0(void);
extern char memcardErrorMessage[];
extern "C" void STUB_printf(const char* fmt, ...);

extern "C" void memcard_Init(void) {
    if (func_001233F0() != 0)
        STUB_printf(memcardErrorMessage);
}

extern "C" int memcard_GetDataSize(int* data) {
    int size = 8;
    while (data[0] != 0) {
        size += 8;
        size += data[1];
        size = (size + 3) & -4;
        data += 4;
    }
    return size + 8;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_Checksum);

extern "C" int memcard_Checksum(int* data, int len);

extern "C" int memcard_TestChecksum(int* data) {
    int stored_crc = data[1];
    int result = 0;
    int len = data[0];
    if (stored_crc != 0)
        result = memcard_Checksum(data + 2, len) == stored_crc;
    return result;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_PrepData);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_RestoreInfo__FPcii);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_RestoreData__FPcT0iP7mc_data);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_Save__Fii);
