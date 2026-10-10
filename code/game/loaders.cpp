#include "common.h"
#include "types.h"
#include "hud.h"

// Level data root (see boot.cpp): levelRoot points at the level's decompressed
// base. Declared without the .data section so the in-gp-window load emits the
// self-based absolute pseudo ps2eeas expands to lui/lw in one register.
struct LevelRoot {
    u32 first_offset;
};

extern LevelRoot* levelRoot;
// Current HUD bank header (&hudHeap.header, 0x19A400). LoadCompressedHudBank
// reads it as a direct symbol; indexing hudHeap.header instead emits a
// base+offset load that does not match.
extern HudHeader* hudHeader __attribute__((section(".data")));
extern "C" void FastDecompress(int, int);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00202270);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_002026C0);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", ParseParticleTexs);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_00202800);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", func_002028E0);

INCLUDE_ASM("code/_generated/nonmatchings/game/loaders", LoadHudBanks__Fv);

// Decompresses HUD bank `i` into `dest`, then clears that bank's bankLoad
// marker. `dest` is rounded up to a 16-byte boundary; when it rounds to zero
// the decompress is skipped. The bank's compressed offset is the word at
// levelRoot + i*8 + 0x28; the decompress source is levelRoot + that offset.
void LoadCompressedHudBank(int i, char* dest) {
    u32 aligned = ((u32)dest + 0xF) & ~0xF;
    if (aligned != 0) {
        u32 base = (u32)(u8*)levelRoot;
        // Split into base + i*8 then the field load: a single
        // base + i*8 + 0x28 expression emits addu v0,v0,a0 instead of the
        // original addu v0,a0,v0.
        u32 idx = base + i * 8;
        u32 offset = *(u32*)(idx + 0x28);
        FastDecompress(offset + base, aligned);
    }
    // bankLoad word at header + i*4 + 0x74 (0x1D words past the header base).
    u32* p = (u32*)((u8*)hudHeader + i * 4);
    p[0x1D] = 0;
}

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
