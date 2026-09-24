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
// C linkage: handwritten fast function in game/fastfunc; returns |x| in f0.
extern "C" float FastAbsF(float x);
// C linkage: handwritten fast vector helpers in game/fastfunc. Vectors are
// 16-byte vec4s (see mobyutil.h); dot/length return their result in f0.
extern "C" void FastVecAdd(void* dest, void* a, void* b);
extern "C" void FastVecSub(void* dest, void* a, void* b);
extern "C" float FastVecDot(void* a, void* b);
extern "C" float FastVecLength(void* a);
extern "C" void FastVecNormalize(void* dest, void* src, float w);
// C linkage: handwritten fast function in game/fastfunc; scales the 16-byte
// vector at src by w (f12), storing xyz at dst and copying w unchanged.
extern "C" void FastVecScale(void* dest, void* src, float w);
// C++ linkage (mangles to FastArcSin__Ff): fast arcsine in game/fastfunc.
float FastArcSin(float x);
#endif

#endif
