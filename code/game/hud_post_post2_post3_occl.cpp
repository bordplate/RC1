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

// The packet's 48-byte payload (tag qcnt 3) is the engine's GS solid-color
// sprite (rectangle fill) record, streamed to the GS by the VU1 chain: a
// 16-byte GIFTAG (type byte 0x44 = solid sprite), a 16-byte attribute block
// (fixed lead word followed by the `attr` word), and a 16-byte corner block
// holding the rect's two packed (x, y) corners. The 0x44 type / 0x4410 param
// word is the engine's solid-sprite tag: the boot ELF's static tag table
// (0x160820-0x160858) holds the sibling tags (0x24/0x14/0xB4) in the same
// layout, and the Deadlocked GIFtag_PrimPixel table is this exact tag with
// its 0x80 flag byte set.
#define OCCL_RECT_GIFTAG 0x4400000000000001ULL
#define OCCL_RECT_GIFTAG_PARAM 0x4410ULL
// Lead word of the sprite attribute block; its meaning is not established in
// the available references.
#define OCCL_RECT_ATTR_LEAD 0x46ULL
// High 32 bits ORed into each packed corner; the 7-arg sibling
// func_00200F90 takes the same value from its caller instead.
#define OCCL_RECT_CORNER_HIGH 0xFFFFF000000000ULL

// 16.0 fixed point in the occlusion view space: occlViewParams.minX/minY are
// pre-scaled by InitViewContext (<< 4). Integer-pixel input is scaled up;
// both forms then take a fixed corner offset (one pixel when scaled, half a
// pixel when already fixed point).
#define OCCL_RECT_X16_SCALE 0x10
#define OCCL_RECT_SCALED_CORNER_OFFSET 0x10
#define OCCL_RECT_RAW_CORNER_OFFSET 8

#define OCCL_RECT_PACK(x, y) ((x) | ((u64)(y) << 16) | OCCL_RECT_CORNER_HIGH)
#define OCCL_RECT_PACK_RAW(px, py) \
    OCCL_RECT_PACK((px) + occlViewParams.minX - OCCL_RECT_RAW_CORNER_OFFSET, \
                   (py) + occlViewParams.minY - OCCL_RECT_RAW_CORNER_OFFSET)
#define OCCL_RECT_PACK_X16(px, py) \
    OCCL_RECT_PACK((px) * OCCL_RECT_X16_SCALE + occlViewParams.minX \
                   - OCCL_RECT_SCALED_CORNER_OFFSET, \
                   (py) * OCCL_RECT_X16_SCALE + occlViewParams.minY \
                   - OCCL_RECT_SCALED_CORNER_OFFSET)

// Appends a 12-word inline VU1 packet (a GS solid-sprite record filling one
// rectangle from corner (x0, y0) to (x1, y1) with the `attr` word) to the
// VU1 chain. When `scale` is set the coordinates arrive as 16.0 fixed point
// and are used raw, otherwise they are integer pixels scaled x16. The packet
// stores go through a plain (non-volatile) `head` so EGC can schedule the
// attr store into the beq delay slot and the second-corner store into the
// fall-through branch's delay slot, as in the original.
void OcclAppendRects(int x0, int y0, int x1, int y1, u64 attr, int scale) {
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
        *(u64*)&head[12] = OCCL_RECT_PACK_RAW(x0, y0);
        *(u64*)&head[14] = OCCL_RECT_PACK_RAW(x1, y1);
    } else {
        *(u64*)&head[12] = OCCL_RECT_PACK_X16(x0, y0);
        *(u64*)&head[14] = OCCL_RECT_PACK_X16(x1, y1);
    }
    vu1ChainHeadStore = vu1ChainHead + 12;
}

// One word of inter-function padding before the next generated function.
asm("nop");
