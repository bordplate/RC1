#include "common.h"
#include "types.h"

typedef struct ReadBuf {
    u8 data[0x50000];
    volatile u32 putPos;
    volatile u32 count;
    volatile u32 capacity;
} ReadBuf;

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/readbuf", readBufCreate__FP7ReadBuf);

void readBufDelete(ReadBuf* self) {}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/readbuf", readBufBeginPut__FP7ReadBufPPUc);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/readbuf", readBufEndPut__FP7ReadBufi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/readbuf", readBufBeginGet__FP7ReadBufPPUc);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/readbuf", readBufEndGet__FP7ReadBufi);
