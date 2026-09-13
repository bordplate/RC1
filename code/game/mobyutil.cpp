#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00212C28);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00212D5C);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00212D68);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00212E20);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00212EC0);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00212F90);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_002130D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00213230);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_002132A0);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_002132A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00213308);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00213358);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_002133D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00213468);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00213580);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_002135F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_002136E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00213920);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00213EA8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00213ED8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00213F30);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214100);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_002141E8);

struct MobyInstance;

void* moby_getActiveObject(struct MobyInstance* m) {
    if (!m)
        return 0;
    int t = *(u16*)((char*)m + 0x34) & 0x20;
    asm volatile("nop\n\t" "nop\n\t" "nop");
    if (t) {
        void* p = *(void**)((char*)m + 0x78);
        return *(void**)p;
    }
    return 0;
}

// Twin of moby_getActiveObject: reads the second pointer (word at +0x10) of
// the per-moby variable block at MobyInstance+0x78, gated on the same 0x20
// mode bit. The caller (func_00213920) writes a flag byte at +0x2E of the
// returned object.
void* moby_getSecondaryObject(struct MobyInstance* m) {
    if (!m)
        return 0;
    int t = *(u16*)((char*)m + 0x34) & 0x20;
    // Three nops: the EE assembler eats one for the beqz delay slot, leaving
    // the original's two.
    asm volatile("nop\n\t" "nop\n\t" "nop");
    if (t) {
        void* p = *(void**)((char*)m + 0x78);
        return *(void**)((char*)p + 0x10);
    }
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214258);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214260);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_002144D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214528);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214530);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214598);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_002146C8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214720);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_002147F8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214800);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214890);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214938);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214970);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214A90);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214BC0);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214C48);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214CC8);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214D58);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214DB0);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214E50);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_002150D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00215130);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_002151D8);

// Per-entity "is active" flag pools, zeroed at boot and populated at runtime.
// The counters below each count the nonzero (active) bytes of one pool and
// clamp the total to a display maximum. The pools' entity domains are only
// labelled by runtime-loaded strings, so the Splat address names are retained.
extern "C" unsigned char D_0013E520[];
extern "C" unsigned char D_0013D408[];

// Unmangled symbols (callers jal the address labels directly), so asm
// labels -- not extern "C" assumptions -- produce them.
int func_00215290(void) asm("func_00215290");
int func_00215300(void) asm("func_00215300");

enum {
    // Maximum active slots the 0x14BEC0 pool reports (the cap applied inside
    // func_00215290 and here).
    MOBY_POOL_MAX_SLOTS = 0x28,
    // Slots of the 0x14BEC0 pool consumed by each active 0x13E520 entry.
    MOBY_POOL_SLOTS_PER_ENTRY = 4,
};

// Free capacity of the 0x14BEC0 moby condition pool: its active slots minus
// the slots consumed by the active 0x13E520 entries, clamped to
// [0, MOBY_POOL_MAX_SLOTS]. The 0x216C48 state machine uses it as a
// slot-index cap and the 0x21EB20 debug HUD displays it. Same
// unmangled-symbol asm-label handling.
int moby_freeVarSlots(void) asm("func_00215248");
int moby_freeVarSlots(void) {
    int n = func_00215290() - MOBY_POOL_SLOTS_PER_ENTRY * func_00215300();
    if (n < 0)
        n = 0;
    if (n > MOBY_POOL_MAX_SLOTS)
        n = MOBY_POOL_MAX_SLOTS;
    return n;
}

// Counts the active entries in the first 0x38 bytes of the 0x14BEC0 pool,
// clamped to [0, 0x28]; the value feeds moby_freeVarSlots and the 0x21EB20
// debug HUD. The pool's entity domain is runtime-loaded, so the address
// name is kept.
INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00215290);

// Counts the active entries in the 0x25-byte pool at 0x13E520, clamped to
// [0, 0xA]. The pool's entity domain is runtime-loaded, so the address name
// is kept.
int func_00215300(void) {
    int i;
    int n = 0;
    for (i = 0; i < 0x25; i++) {
        if (D_0013E520[i])
            n++;
    }
    if (n < 0)
        n = 0;
    if (n > 0xA)
        n = 0xA;
    return n;
}

// Sibling of func_00215300: counts the active entries in the 0x20-byte pool
// at 0x13D408, clamped to [0, 0x1E]. Same unmangled-symbol asm-label handling;
// the pool's entity domain is runtime-loaded, so the address name is kept.
int func_00215348(void) asm("func_00215348");
int func_00215348(void) {
    int i;
    int n = 0;
    for (i = 0; i < 0x20; i++) {
        if (D_0013D408[i])
            n++;
    }
    if (n < 0)
        n = 0;
    if (n > 0x1E)
        n = 0x1E;
    return n;
}
