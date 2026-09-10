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

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00214228);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00215248);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00215290);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00215300);

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyutil", func_00215348);
