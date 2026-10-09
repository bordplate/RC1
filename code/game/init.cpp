#include "common.h"
#include "types.h"

// SIF DMA transfer descriptor used by sceSifSetDma/sceSifDmaStat: `data` is
// the EE-side buffer, `addr` the IOP-side destination.
typedef struct {
    u32 data;
    u32 addr;
    s32 size;
    s32 mode;
} SifDmaData;

// C linkage: syscall veneer in generated sce/lib.s at 0x0011C8C8; the
// original call targets the unmangled SDK symbol.
extern "C" int sceSifAllocIopHeap(int size);
// C linkage: syscall veneer in generated sce/lib.s at 0x0011C9B0; the
// original call targets the unmangled SDK symbol.
extern "C" void sceSifFreeIopHeap(int addr);
// C linkage: syscall veneer in generated sce/lib.s at 0x00118B20; the
// original call targets the unmangled SDK symbol.
extern "C" int sceSifSetDma(SifDmaData* dma, int count);
// C linkage: syscall veneer in generated sce/lib.s at 0x00118B10; the
// original call targets the unmangled SDK symbol.
extern "C" int sceSifDmaStat(int id);
// C linkage: syscall veneer in generated sce/lib.s at 0x0011CD78; the
// original call targets the unmangled SDK symbol.
extern "C" int sceSifLoadModuleBuffer(int addr, int b, int c);

// Symbol override: the module-load entry point is the unmangled symbol
// LoadIRXModule in the boot ELF; a C++ free function here would mangle
// differently, so pin it with a symbol override instead of assuming C
// linkage.
int LoadIRXModule(int src, int size) asm("LoadIRXModule");

int LoadIRXModule(int src, int size) {
    int dst = sceSifAllocIopHeap(size);

    SifDmaData dma;
    dma.data = src;
    dma.addr = dst;
    dma.size = size;
    dma.mode = 0;

    int id = sceSifSetDma(&dma, 1);
    int ok = 1;

    if (id != 0) {
        int stat;
        do {
            stat = sceSifDmaStat(id);
        } while (stat >= 0);

        int r = sceSifLoadModuleBuffer(dst, 0, 0);

        // The original tests (r >= 0) once and reuses the slt result for the
        // movz scheduled in the sceSifFreeIopHeap delay slot; a plain `r < 0`
        // compiles to a movn and spills r across the call.
        if (!(r >= 0)) {
            ok = 0;
        }

        sceSifFreeIopHeap(dst);
    }

    return ok;
}
