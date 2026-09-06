typedef struct ViBuf {
    unsigned int base;
    unsigned int tags;
    unsigned int blocks;
} ViBuf;

unsigned int getFIFOindex(ViBuf* self, void* v) {
    unsigned int i = ((self->blocks << 4) + self->tags + 0x10) & 0xFFFFFFF;
    if (v == (void*)i) {
        return 0;
    }
    return ((unsigned int)v - self->base) >> 11;
}
