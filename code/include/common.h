#ifndef COMMON_H
#define COMMON_H

#include "include_asm.h"

#ifdef __cplusplus
// C linkage: these runtime and SDK helpers are unmangled entry points.
extern "C" void FlushCache(int arg1);
extern "C" void FastMemSet(void* p1, int p2, int p3);
extern "C" void FastMemCopy(void* p1, void* p2, int p3);
// C linkage: handwritten VU fast function in game/fastfunc; returns the
// 3D distance between the two vectors in f0.
extern "C" float FastVecDist(void* p1, void* p2);
#endif

#endif
