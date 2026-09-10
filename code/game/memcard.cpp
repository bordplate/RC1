#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_GetName);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", func_00209168);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_RestoreGame);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", func_00209370);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_Update);

INCLUDE_ASM("code/_generated/nonmatchings/game/memcard", memcard_MakeWholeSave);

// C linkage: this memory-card routine is implemented in generated sce/lib.s
// and memcard_Init calls its unmangled entry point at 0x001233F0.
extern "C" int memcard_queryStatus(void);
extern char memcardErrorMessage[];
// C linkage: this diagnostic entry point is supplied by generated sce/lib.s.
extern "C" void STUB_printf(const char* fmt, ...);

// C linkage: this routine is called from the original unmangled memcard API.
extern "C" void memcard_Init(void) {
    if (memcard_queryStatus() != 0)
        STUB_printf(memcardErrorMessage);
}

// C linkage: this routine is called from the original unmangled memcard API.
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

// C linkage: the checksum implementation remains supplied by generated
// assembly at the original unmangled entry point.
extern "C" int memcard_Checksum(int* data, int len);

// C linkage: this routine is called from the original unmangled memcard API.
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
