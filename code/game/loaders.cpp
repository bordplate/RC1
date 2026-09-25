#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00202270);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_002026C0);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", ParseParticleTexs);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00202800);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_002028E0);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", LoadHudBanks__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", LoadCompressedHudBank__FiPc);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00202D78);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", SetUpVisGifViewer__FPiiiiii);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00203120);

// Loaded data chunks store their embedded pointers as offsets relative to the
// chunk base. After a chunk is placed, this converts them to absolute addresses:
// the single pointer at +0x14 (if set) and the relocCount pointers at +0x1C.
// obj is the chunk pointer stored at base[index*4 + 0x48].
struct RelocChunk {
    u8 pad_00[0x10];
    u8 relocCount; // +0x10
    u8 pad_11[3];
    s32 firstReloc; // +0x14, offset from chunk base
    u8 pad_18[4];
};

// Callers are generated asm referencing the unmangled symbol; keep the label.
void relocateObjectPointers(u32 base, s32 index) asm("relocateObjectPointers");

void relocateObjectPointers(u32 base, s32 index) {
    u8* p = (u8*)base + index * 4 + 0x48;
    struct RelocChunk* obj = (struct RelocChunk*)*(u32*)p;
    if (obj->firstReloc != 0) {
        obj->firstReloc = (s32)(u8*)obj + obj->firstReloc;
    }
    if ((s32)(u8)obj->relocCount != 0) {
        s32 i = 0;
        s32* arr = (s32*)((u8*)obj + 0x1C);
        do {
            *arr = (s32)(u8*)obj + *arr;
            i++;
            arr++;
        } while (i < (s32)(u8)obj->relocCount);
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00203338);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00203640);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00203730);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00203B08);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_002040E0);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_002043B0);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00204428);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00204788);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00204790);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_002049E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", ParseSpaceSceneChunk__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00204A40);
