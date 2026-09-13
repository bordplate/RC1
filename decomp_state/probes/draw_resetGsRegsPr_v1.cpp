#include "types.h"

// GS frame-buffer configuration values; zero-initialized, written only by
// level code outside the boot image.
struct GsDisplayParams {
    u64 f0;
    u64 f8;
    u64 f10;
};

extern struct GsDisplayParams gsDisplayParams;

void ResetGsRegistersPr() {
    *(u64*)0x120000E0 = 0;
    *(u64*)0x12000000 = 0xFFA1;
    *(u64*)0x12000020 = gsDisplayParams.f0;
    *(u64*)0x12000070 = gsDisplayParams.f8;
    *(u64*)0x12000090 = gsDisplayParams.f8;
    *(u64*)0x12000080 = gsDisplayParams.f10;
    *(u64*)0x120000A0 = gsDisplayParams.f10;
    *(u64*)0x120000D0 = 0;
}
