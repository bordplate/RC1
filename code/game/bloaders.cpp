#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/bloaders", LoadPifAsPSMT8H);

extern "C" int Load(u8* a0, int a1, int a2);
extern "C" void LoadPifAsPSMT8H(u8* a0, u8* a1, int a2, int a3);

typedef struct {
    u8 pad[8];
    u32 src;
    u32 size;
} DebugFontLoadInfo;

extern DebugFontLoadInfo debugFontLoadInfo __attribute__((section(".data")));
extern u8 debugFontBuffer[] __attribute__((section(".data")));

extern "C" void LoadDebugFont(void) {
    u8 buf[0x18];
    Load(debugFontBuffer, debugFontLoadInfo.src, debugFontLoadInfo.size);
    LoadPifAsPSMT8H(debugFontBuffer, buf, *(u32*)0x15EE88 + 0xC0000, 0x3FFC00);
    *(u64*)0x15EEC8 = *(u64*)buf;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/bloaders", func_001E93A8);
