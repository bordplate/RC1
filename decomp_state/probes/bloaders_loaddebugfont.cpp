#include "common.h"
#include "types.h"

extern "C" int Load(u8* a0, int a1, int a2);
extern "C" void LoadPifAsPSMT8H(u8* a0, u8* a1, int a2, int a3);

typedef struct {
    u8 pad[8];
    u32 src;
    u32 size;
} DebugFontLoadInfo;

extern "C" DebugFontLoadInfo D_00137B80 __attribute__((section(".data")));
extern "C" u8 D_001AABC0[] __attribute__((section(".data")));

extern "C" void LoadDebugFont(void) {
    u8 buf[0x18];
    Load(D_001AABC0, D_00137B80.src, D_00137B80.size);
    LoadPifAsPSMT8H(D_001AABC0, buf, *(u32*)0x15EE88 + 0xC0000, 0x3FFC00);
    *(u64*)0x15EEC8 = *(u64*)buf;
}
