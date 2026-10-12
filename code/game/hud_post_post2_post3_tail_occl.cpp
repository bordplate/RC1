#include "common.h"
#include "types.h"
#include "camera.h"

// Double volatile: EGC must re-read the VU1 chain head pointer before every
// packet store; a plain or single-volatile declaration lets it CSE the later
// loads into one shared base. This TU is assembled with ps2eeas, which expands
// each bare `lw r, vu1ChainHead` pseudo into a fresh self-based lui/lw pair,
// matching the original. (A .data attribute or a GNU-assembled TU breaks that.)
extern volatile u32* volatile vu1ChainHead;

// VIF tag for the inline VU1 packet this function appends.
#define VU1_INLINE_PKT_TAG 0x10000000u
#define VU1_DATA_REF_END_TAG 0x50000000u

// The packet's payload is the engine's GS solid-color sprite (rectangle fill)
// record, identical to the sibling OcclAppendRects: a 16-byte GIFTAG (type
// byte 0x44 = solid sprite), a 16-byte attribute block (fixed lead word
// followed by the `attr` word), and a 16-byte corner block holding the rect's
// two packed (x, y) corners. See notes/hud_post_post2_post3_occl_OcclAppendRects.md.
#define OCCL_RECT_GIFTAG 0x4400000000000001ULL
#define OCCL_RECT_GIFTAG_PARAM 0x4410ULL
// Lead word of the sprite attribute block; its meaning is not established in
// the available references.
#define OCCL_RECT_ATTR_LEAD 0x46ULL

// 16.0 fixed point in the occlusion view space: occlViewParams.minX/minY are
// pre-scaled by InitViewContext (<< 4). Integer-pixel input is scaled up;
// both forms then take a fixed corner offset (one pixel when scaled, half a
// pixel when already fixed point).
#define OCCL_RECT_X16_SCALE 0x10
#define OCCL_RECT_SCALED_CORNER_OFFSET 0x10
#define OCCL_RECT_RAW_CORNER_OFFSET 8

// Appends a 12-word inline VU1 packet (a GS solid-sprite record filling one
// rectangle from corner (x0, y0) to (x1, y1) with the `attr` word) to the
// VU1 chain, the 7-arg sibling of OcclAppendRects. Unlike the sibling, the
// high 32 bits ORed into each packed corner are the `cornerHi` argument
// instead of the constant 0xFFFFF000 (the sole boot caller passes 0). When
// `scale` is set the coordinates arrive as 16.0 fixed point and are used
// raw, otherwise they are integer pixels scaled x16. The packet stores go
// through a plain (non-volatile) `head` so EGC can schedule the attr store
// into the beq delay slot and the second-corner store into the
// fall-through branch's delay slot, as in the original.
void OcclAppendRectsHi(int x0, int y0, int x1, int y1, u64 attr, u32 cornerHi, int scale) {
    vu1ChainHead[0] = VU1_INLINE_PKT_TAG | 3;
    vu1ChainHead[1] = 0;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 3;
    u32* head = (u32*)vu1ChainHead;
    u32* next = head + 4;
    vu1ChainHead = next;
    *(u64*)&head[4] = OCCL_RECT_GIFTAG;
    *(u64*)&head[6] = OCCL_RECT_GIFTAG_PARAM;
    *(u64*)&head[8] = OCCL_RECT_ATTR_LEAD;
    *(u64*)&head[10] = attr;
    if (scale) {
        u64 high = (u64)cornerHi << 32;
        *(u64*)&head[12] = (x0 + occlViewParams.minX - OCCL_RECT_RAW_CORNER_OFFSET) |
                           ((u64)(y0 + occlViewParams.minY - OCCL_RECT_RAW_CORNER_OFFSET) << 16) |
                           high;
        *(u64*)&head[14] = (x1 + occlViewParams.minX - OCCL_RECT_RAW_CORNER_OFFSET) |
                           ((u64)(y1 + occlViewParams.minY - OCCL_RECT_RAW_CORNER_OFFSET) << 16) |
                           high;
    } else {
        u64 high = (u64)cornerHi << 32;
        *(u64*)&head[12] = (x0 * OCCL_RECT_X16_SCALE + occlViewParams.minX -
                            OCCL_RECT_SCALED_CORNER_OFFSET) |
                           ((u64)(y0 * OCCL_RECT_X16_SCALE + occlViewParams.minY -
                                  OCCL_RECT_SCALED_CORNER_OFFSET) << 16) |
                           high;
        *(u64*)&head[14] = (x1 * OCCL_RECT_X16_SCALE + occlViewParams.minX -
                            OCCL_RECT_SCALED_CORNER_OFFSET) |
                           ((u64)(y1 * OCCL_RECT_X16_SCALE + occlViewParams.minY -
                                  OCCL_RECT_SCALED_CORNER_OFFSET) << 16) |
                           high;
    }
    vu1ChainHead = (u32*)vu1ChainHead + 12;
}
