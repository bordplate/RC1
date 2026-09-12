#include "common.h"
#include "types.h"

typedef struct ReadBuf {
    u8 data[0x50000];
    int putPos;
    int count;
    int capacity;
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

void readBufEndPut(ReadBuf* buf, int n) {
    int space = buf->capacity - buf->count;
    int m = (n < space) ? n : space;
    buf->putPos = (buf->putPos + m) % buf->capacity;
    buf->count += m;
}

int readBufBeginGet(ReadBuf* buf, u8** out) {
    if (buf->count) {
        *out = buf->data + ((buf->putPos - buf->count) + buf->capacity) % buf->capacity;
    }
    return buf->count;
}

int readBufEndGet(ReadBuf* buf, int n) {
    int ret = (n < buf->count) ? n : buf->count;
    buf->count -= ret;
    return ret;
}
