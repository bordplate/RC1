#include "common.h"
#include "types.h"

typedef struct VoBuf {
    void* data;
    void* tags;
    volatile u32 head;
    volatile int count;
    u32 capacity;
} VoBuf;

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufCreate__FP5VoBufP6VoDataP5VoTagi);

void voBufDelete(VoBuf* self) {}

extern "C" void voBufReset__FP5VoBuf(VoBuf* self) {
    self->count = 0;
    self->head = 0;
}

extern "C" int voBufIsFull__FP5VoBuf(VoBuf* self) {
    return self->count == self->capacity;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufIncCount__FP5VoBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufGetData__FP5VoBuf);

extern "C" int voBufIsEmpty(VoBuf* self) {
    return self->count == 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufGetTag__FP5VoBuf);

void voBufDecCount(VoBuf* self) {
    if (self->count > 0) {
        self->count--;
    }
}
