#ifndef BOOT_LEVEL_H
#define BOOT_LEVEL_H

#include "types.h"

struct BootStreamInfo {
    int src;
    int size;
};

// Resident boot asset table. The font loader uses the first record; startlevel
// uses the bank's disc location at +0x14E0. InitOnce uses the IRX record at
// +0x12C0 (disc sector offset) / +0x12C4 (sector count) to stage and load the
// IOP modules. Other records remain unidentified.
struct BootAssetTable {
    u8 pad_0x00[8];
    u32 src;
    u32 size;
    u8 pad_0x10[0x4E8];
    // Level-archive record: func_002043B0 derives the level-root placement from
    // sectors (each sector 0x800 bytes) and passes both fields to the stream
    // setup (func_00216728); dest is the archive destination word.
    s32 dest;
    s32 sectors;
    u8 pad_0x500[0xDC0];
    u32 irxSector;
    u32 irxCount;
    u8 pad_0x12C8[0x218];
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
