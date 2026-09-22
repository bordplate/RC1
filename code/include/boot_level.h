#ifndef BOOT_LEVEL_H
#define BOOT_LEVEL_H

#include "types.h"

struct BootStreamInfo {
    int src;
    int size;
};

// Resident boot asset table. The font loader uses the first record; startlevel
// uses the bank's disc location at +0x14E0. Other records remain unidentified.
struct BootAssetTable {
    u8 pad_0x00[8];
    u32 src;
    u32 size;
    u8 pad_0x10[0x14D0];
    int soundBankLocation;
};

extern BootAssetTable bootAssets __attribute__((section(".data")));

// Offsets in the decompressed boot archive, relative to the archive base.
struct BootImageRef {
    int offset;
    int size;
};

struct BootImageArchive {
    BootImageRef splash[2];
    BootImageRef cardWarnings[2][6];
    BootImageRef decodeBuffer;
};

#endif
