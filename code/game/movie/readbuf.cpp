#include "common.h"
#include "types.h"

typedef struct ReadBuf {
    u8 data[0x50000];
    u32 putPos;
    u32 count;
    u32 capacity;
} ReadBuf;

void readBufCreate(ReadBuf* self) {
    self->capacity = 0x50000;
    self->putPos = 0;
    self->count = 0;
}

void readBufDelete(ReadBuf* self) {}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/readbuf", readBufBeginPut__FP7ReadBufPPUc);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/readbuf", readBufEndPut__FP7ReadBufi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/readbuf", readBufBeginGet__FP7ReadBufPPUc);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/readbuf", readBufEndGet__FP7ReadBufi);
