#include "common.h"
#include "types.h"

typedef struct ReadBuf {
    u8 data[0x50000];
    u32 putPos;
    int count;
    u32 capacity;
} ReadBuf;

void readBufCreate(ReadBuf* self) {
    self->capacity = 0x50000;
    self->putPos = 0;
    self->count = 0;
}

void readBufDelete(ReadBuf* self) {}

int readBufBeginPut(ReadBuf* buf, u8** out) {
    int space = buf->capacity - buf->count;
    if (space) {
        *out = buf->data + buf->putPos;
    }
    return space;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/readbuf", readBufEndPut__FP7ReadBufi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/readbuf", readBufBeginGet__FP7ReadBufPPUc);

int readBufEndGet(ReadBuf* buf, int n) {
    int ret = (n < buf->count) ? n : buf->count;
    buf->count -= ret;
    return ret;
}
