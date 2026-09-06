#include "common.h"
#include "types.h"
#include "vibuf.h"

typedef struct VideoDec {
    u8 _pad[0x48];
    ViBuf vibuf;
    u32 state;
} VideoDec;

int viBufCount(ViBuf* buf);

int viBufBeginPut(ViBuf* buf, u8** data, int* size, u8** data2, int* size2);

struct sceMpeg;
struct sceMpegCbData;
struct sceMpegCbDataError;

typedef int (*videoDecCallback)(struct sceMpeg*, struct sceMpegCbData*, void*);

extern "C" int func_0012AEC8(VideoDec* self, int a, int b,
    videoDecCallback cb, void* userdata);

void viBufDelete(ViBuf* self);
extern "C" int func_0012B9E0(void* p);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecCreate__FP8VideoDecPUciPUxT3iP9TimeStampi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023CBC8);

int videoDecSetStream(VideoDec* self, int a, int b, videoDecCallback cb, void* userdata) {
    func_0012AEC8(self, a, b, cb, userdata);
    return 1;
}

int videoDecBeginPut(VideoDec* self, u8** data, int* size, u8** data2, int* size2) {
    return viBufBeginPut(&self->vibuf, data, size, data2, size2);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecEndPut__FP8VideoDec);

void videoDecReset(VideoDec* self) {
    self->state = 0;
}

int videoDecDelete(VideoDec* self) {
    viBufDelete(&self->vibuf);
    func_0012B9E0(self);
    return 1;
}

void videoDecAbort(VideoDec* self) {
    self->state = 1;
}

extern "C" int videoDecGetState(VideoDec* self) {
    return self->state;
}

u32 videoDecSetState(VideoDec* self, u32 state) {
    u32 old = self->state;
    self->state = state;
    return old;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecPutTs__FP8VideoDecllPUci);

int videoDecInputCount(VideoDec* self) {
    return viBufCount(&self->vibuf);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023CD00);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecFlush__FP8VideoDec);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecIsFlushed__FP8VideoDec);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecMain__FPv);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", decBs0__FP8VideoDec);

extern char D_00161220[];
extern "C" void STUB_printf(const char* fmt, ...);

int mpegError(struct sceMpeg* mpeg, struct sceMpegCbDataError* err, void* user) {
    STUB_printf(D_00161220, *(int*)((char*)err + 4));
    return 1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", mpegNodata__FP7sceMpegP13sceMpegCbDataPv);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023D0E0);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023D110);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023D140);
