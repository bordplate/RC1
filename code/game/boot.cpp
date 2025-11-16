#include "common.h"
#include "types.h"

struct LevelLoad {
    u8* dst;
    u32 len;
    u32 src_off;
    void* entry;
};

struct LevelRoot {
    u32 first_offset;
};

typedef void (*StartLevelPtr)(void);

extern LevelRoot* DAT_0015EE4C __attribute__((section(".data")));

void startlevel();
StartLevelPtr ParseBin();

#ifndef ALLOW_NONMATCHING
INCLUDE_ASM("code/_generated/nonmatchings/game/boot", ParseBin__Fv);
#endif

#if defined(SKIP_ASM) || defined(ALLOW_NONMATCHING)

StartLevelPtr ParseBin() {
    void* entrypoint = nullptr;
    u8* base = (u8*)DAT_0015EE4C + *(u32*)DAT_0015EE4C;
    LevelLoad* chunk = (LevelLoad*)(base);

    while (true) {
        u8* dst = chunk->dst;
        u32 len = chunk->len;
        u8* src = (u8*)chunk + 0x10;

        if (entrypoint == nullptr) {
            entrypoint = chunk->entry;
        } else if (entrypoint != chunk->entry) {
            break;
        }

        if ((((u32)dst | (u32)src | len) & 7) == 0) {
            u8* end = dst + len;
            u64* d = (u64*)dst;
            u64* s = (u64*)src;

            while ((u8*)d != end) {
                *d++ = *s++;
            }
        } else {
            u8* end = dst + len;
            u32* d = (u32*)dst;
            u32* s = (u32*)src;

            while ((u8*)d != end) {
                *d++ = *s++;
            }
        }

        chunk = (LevelLoad*)((u32)src + len);
    }

#ifndef SKIP_ASM
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
#endif

    return (StartLevelPtr)entrypoint;
}

#endif

int main(int argc, char **argv) {
    StartLevelPtr pcVar1 = startlevel;

    while (true) {
        pcVar1();
        pcVar1 = ParseBin();
        FlushCache(0);
        FlushCache(2);
    }
}
