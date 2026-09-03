#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufCreate__FP5VoBufP6VoDataP5VoTagi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufDelete__FP5VoBuf);

typedef struct VoBuf {
    void* data;
    void* tags;
    volatile u32 head;
    volatile u32 count;
    u32 capacity;
} VoBuf;

extern "C" void voBufReset__FP5VoBuf(VoBuf* self) {
    self->count = 0;
    self->head = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufIsFull__FP5VoBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufIncCount__FP5VoBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufGetData__FP5VoBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufIsEmpty);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufGetTag__FP5VoBuf);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufDecCount__FP5VoBuf);
