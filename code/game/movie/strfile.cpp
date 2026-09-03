#include "common.h"
#include "types.h"

typedef struct StrFile {
    u8 _pad[8];
} StrFile;

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/strfile", func_0023BA48);

int strFileDelete(StrFile* self) {
    return 1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/strfile", func_0023BA60);
