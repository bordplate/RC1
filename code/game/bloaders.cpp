#include "common.h"
#include "types.h"
#include "boot_level.h"

// PIF (PSM T8H) image header read from the debug font buffer.
typedef struct {
    int pifID;
    int fileSize;
    int uSize;
    int vSize;
    int texFormat;
    int clutFormat;
    int clutOrder;
    int mipLevels;
} PifHeader;

// Parsed image descriptor (0x54 bytes) built on the stack by the loader.
typedef struct {
    void* pClut;
    void* pTex[4];
    int cSize;
    int tSize[4];
    int cPos;
    int tPos[4];
    int tbw[4];
    int uLog;
    int vLog;
} PifParser;

// 96-byte GS load-image packet built by sceGsSetDefLoadImage.
typedef struct {
    u8 _data[96];
} sceGsLoadImage;

// C linkage: SCE GS image loader entry points (core.text SDK code). All three
// return int (status/result); the ignored return value still affects EGC
// register allocation, so the prototypes must keep the int return type.
extern "C" int sceGsSetDefLoadImage(sceGsLoadImage* image, short p1, short p2,
                                    short p3, short p4, short p5, short p6,
                                    short p7);
extern "C" int sceGsExecLoadImage(sceGsLoadImage* image, void* source);
extern "C" int sceGsSyncPath(int path, int sync);
extern "C" int log2dim(int x);

// C linkage: these loader routines are SDK-style unmangled entry points.
extern "C" int Load(u8* source, int offset, int size);

// C linkage: PSM T8H image loader. Builds the GS load packet in image and the
// 16-byte PIF header in destination from the PifHeader at source.
extern "C" void LoadPifAsPSMT8H(u8* source, u8* destination, int offset, int size);
//
// BLOCKED (see decomp_state/blocked.json): a C candidate reproduces the whole
// body byte-for-byte (both log2dim calls with the delay-slot palShift/uLog,
// the size-register reuse as the image base, the dead uSize*vSize mult that
// stores uSize, the two GS load sequences, and the 64-bit PIF-header OR chain),
// but EGC 2.95.2 emits the prologue sq/move sequence with s1 before s0,
// whereas the original has s0 before s1. Register pinning ($16-$20) fixes the
// s-register assignment but not the prologue schedule; -fno-schedule-insns and
// declaration reordering do not change it. Retain the generated assembly.
INCLUDE_ASM("code/_generated/nonmatchings/game/bloaders", LoadPifAsPSMT8H);

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
    Load(debugFontBuffer, bootAssets.src, bootAssets.size);
    LoadPifAsPSMT8H(debugFontBuffer, buf, frameBufferBase + 0xC0000, 0x3FFC00);
    debugFontPifHeader = *(u64*)buf;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/bloaders", func_001E93A8);
