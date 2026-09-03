#include "common.h"
#include "types.h"

typedef struct StrFile {
    u32 frameCount;
    u32 offset;
} StrFile;

extern "C" int func_0023BA48(StrFile* self, int offset, int frameCount) {
    self->offset = offset;
    self->frameCount = frameCount;
    return 1;
}

int strFileDelete(StrFile* self) {
    return 1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/strfile", func_0023BA60);
