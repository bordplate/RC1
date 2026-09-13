#include "common.h"
#include "types.h"

typedef struct VoBuf {
    void* data;
    void* tags;
    volatile u32 head;
    volatile int count;
    u32 capacity;
} VoBuf;

// Referenced by pointer only; full layouts not yet determined.
typedef struct VoData {
} VoData;
typedef struct VoTag {
} VoTag;

// Each tag slot occupies this many bytes; only the leading word is initialized.
#define VO_TAG_SLOT_STRIDE 0x138C0

void voBufCreate(VoBuf* self, VoData* data, VoTag* tags, int size) {
    self->count = 0;
    self->data = data;
    self->tags = tags;
    self->capacity = size;
    self->head = 0;
    if (size > 0) {
        int offset = 0;
        int i = size;
        while (i) {
            // Integer add keeps the byte offset as the addu base (rs), matching
            // the original; pointer arithmetic would reorder the operands.
            *(u32*)((u8*)(offset + (u32)self->tags)) = 0;
            offset += VO_TAG_SLOT_STRIDE;
            i--;
        }
    }
}

void voBufDelete(VoBuf* self) {}

// C linkage: this helper retains the original unmangled generated symbol.
extern "C" void voBufReset__FP5VoBuf(VoBuf* self) {
    self->count = 0;
    self->head = 0;
}

// C linkage: this helper retains the original unmangled generated symbol.
extern "C" int voBufIsFull__FP5VoBuf(VoBuf* self) {
    return self->count == self->capacity;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufIncCount__FP5VoBuf);

void* voBufGetData(VoBuf* self) {
    if (voBufIsFull__FP5VoBuf(self)) {
        return 0;
    }
    return (void*)((char*)self->data + self->head * 0xD0000);
}

// C linkage: this helper is called through the original unmangled API.
extern "C" int voBufIsEmpty(VoBuf* self) {
    return self->count == 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/vobuf", voBufGetTag__FP5VoBuf);

void voBufDecCount(VoBuf* self) {
    if (self->count > 0) {
        self->count--;
    }
}
