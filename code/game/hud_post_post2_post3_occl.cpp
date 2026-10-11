#include "common.h"
#include "types.h"
#include "hud.h"
#include "camera.h"

// Double volatile: EGC must re-read the VU1 chain head pointer before every
// packet store; a plain or single-volatile declaration lets it CSE the later
// loads into one shared base. This TU is assembled with ps2eeas, which expands
// each bare `lw r, vu1ChainHead` pseudo into a fresh self-based lui/lw pair,
// matching the original. (A .data attribute or a GNU-assembled TU breaks that.)
extern volatile u32* volatile vu1ChainHead;
// Same object as vu1ChainHead: the plain alias keeps the GPREL form of the
// final head update, which sits in the jr $ra delay slot.
extern volatile u32* vu1ChainHeadStore;

// VIF tag for the inline VU1 packet this function appends.
#define VU1_INLINE_PKT_TAG 0x10000000u
#define VU1_DATA_REF_END_TAG 0x50000000u

// Appends a 12-word inline VU1 packet (GSGX/ADDA/LQ setup plus two packed
// occlusion rects) to the VU1 chain. Each rect packs (x, y) as
// (x & 0xFFF) | ((y & 0xFFF) << 16) | 0xFFFFF000000000, offset into view space
// by occlViewParams.minX/minY; when `scale` is set the coords are used raw
// (minus 8), otherwise x16 16.0 fixed-point (x * 16, minus 16). The packet
// stores go through a plain (non-volatile) `head` so EGC can schedule the attr
// store into the beq delay slot and the second rect store into the
// fall-through branch's delay slot, as in the original.
void OcclAppendRects(int x0, int y0, int x1, int y1, u64 attr, int scale) {
    vu1ChainHead[0] = VU1_INLINE_PKT_TAG | 3;
    vu1ChainHead[1] = 0;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 3;
    u32* head = (u32*)vu1ChainHead;
    u32* next = head + 4;
    vu1ChainHead = next;
    *(u64*)&head[4] = 0x4400000000000001ULL;
    *(u64*)&head[6] = 0x4410;
    *(u64*)&head[8] = 0x46;
    *(u64*)&head[10] = attr;
    if (scale) {
        *(u64*)&head[12] =
            (x0 + occlViewParams.minX - 8) | ((u64)(y0 + occlViewParams.minY - 8) << 16) | 0xFFFFF000000000ULL;
        *(u64*)&head[14] =
            (x1 + occlViewParams.minX - 8) | ((u64)(y1 + occlViewParams.minY - 8) << 16) | 0xFFFFF000000000ULL;
    } else {
        *(u64*)&head[12] =
            (x0 * 0x10 + occlViewParams.minX - 0x10) | ((u64)(y0 * 0x10 + occlViewParams.minY - 0x10) << 16) | 0xFFFFF000000000ULL;
        *(u64*)&head[14] =
            (x1 * 0x10 + occlViewParams.minX - 0x10) | ((u64)(y1 * 0x10 + occlViewParams.minY - 0x10) << 16) | 0xFFFFF000000000ULL;
    }
    vu1ChainHeadStore = vu1ChainHead + 12;
}

// One word of inter-function padding before the next generated function.
asm("nop");
