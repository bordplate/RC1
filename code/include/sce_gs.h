#ifndef SCE_GS_H
#define SCE_GS_H

#include "types.h"

// 96-byte GS load-image packet built by sceGsSetDefLoadImage (it writes the
// first 88 bytes); SetPalMode keeps one on the stack as the upload source.
typedef struct {
    u8 _data[96];
} sceGsLoadImage;

// C linkage: SCE GS image loader entry points (core.text SDK code). All three
// return int (status/result); the ignored return value still affects EGC
// register allocation, so the prototypes must keep the int return type. The
// generated assembly labels them func_00120558/00122330/00122658; the real
// names resolve through config/linker_aliases.ld.
extern "C" int sceGsSyncPath(int path, int sync);
extern "C" int sceGsSetDefLoadImage(sceGsLoadImage* image, short p1, short p2,
                                    short p3, short p4, short p5, short p6,
                                    short p7);
extern "C" int sceGsExecLoadImage(sceGsLoadImage* image, void* source);

#endif
