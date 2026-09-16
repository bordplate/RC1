#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/bloaders", LoadPifAsPSMT8H);

// C linkage: these loader routines are SDK-style unmangled entry points.
extern "C" int Load(u8* source, int offset, int size);
// C linkage: this loader routine is supplied by generated assembly at its
// original unmangled entry point.
extern "C" void LoadPifAsPSMT8H(u8* source, u8* destination, int offset, int size);

typedef struct {
    u8 pad[8];
    u32 src;
    u32 size;
} DebugFontLoadInfo;

extern DebugFontLoadInfo debugFontLoadInfo __attribute__((section(".data")));
extern u8 debugFontBuffer[] __attribute__((section(".data")));
// Match-sensitive: these stay PLAIN externs (no .data section attribute).
// EGC then emits one GPREL pseudo per access, and ps2eeas — which sees the
// reference before the end-of-file .extern — expands it in place to the
// original's self-based absolute lui/lw (load) and lui at/sd (store). A .data
// attribute makes EGC split the address into two schedulable instructions,
// which reorders and changes the value registers.
//
// Base of the GS frame buffer (0x1B0000 or 0x1E0000, set by the resolution
// setup code); used to position the debug font's draw packet.
extern u32 frameBufferBase;
// First quadword of the debug font PIF header built by LoadPifAsPSMT8H.
extern u64 debugFontPifHeader;

// C linkage: this loader entry point is called by the original boot code.
extern "C" void LoadDebugFont(void) {
    u8 buf[0x18];
    Load(debugFontBuffer, debugFontLoadInfo.src, debugFontLoadInfo.size);
    LoadPifAsPSMT8H(debugFontBuffer, buf, frameBufferBase + 0xC0000, 0x3FFC00);
    debugFontPifHeader = *(u64*)buf;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/bloaders", func_001E93A8);
