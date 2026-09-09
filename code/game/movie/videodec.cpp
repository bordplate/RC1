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

void viBufEndPut(ViBuf* buf) asm("viBufEndPut__FP5ViBufi");

struct sceMpeg;
struct sceMpegCbData;
struct sceMpegCbDataError;

typedef int (*videoDecCallback)(struct sceMpeg*, struct sceMpegCbData*, void*);

extern "C" int func_0012AEC8(VideoDec* self, int a, int b,
    videoDecCallback cb, void* userdata);

void viBufDelete(ViBuf* self);
void viBufAddDMA(ViBuf* buf);
extern "C" int func_0012B9E0(void* p);
extern "C" void switchThread(void);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecCreate__FP8VideoDecPUciPUxT3iP9TimeStampi);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023CBC8);

int videoDecSetStream(VideoDec* self, int a, int b, videoDecCallback cb, void* userdata) {
    func_0012AEC8(self, a, b, cb, userdata);
    return 1;
}

int videoDecBeginPut(VideoDec* self, u8** data, int* size, u8** data2, int* size2) {
    return viBufBeginPut(&self->vibuf, data, size, data2, size2);
}

void videoDecEndPut(VideoDec* self) {
    viBufEndPut(&self->vibuf);
}

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

extern "C" unsigned int func_0012BA58(VideoDec* self);

int videoDecIsFlushed(VideoDec* self) {
    int ret = 0;
    if (videoDecInputCount(self) == 0)
        ret = func_0012BA58(self) > 0;
    return ret;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", videoDecMain__FPv);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", decBs0__FP8VideoDec);

extern char D_00161220[];
extern "C" void STUB_printf(const char* fmt, ...);

int mpegError(struct sceMpeg* mpeg, struct sceMpegCbDataError* err, void* user) {
    STUB_printf(D_00161220, *(int*)((char*)err + 4));
    return 1;
}

int mpegNodata(struct sceMpeg* mpeg, struct sceMpegCbData* cbData, void* user) {
    switchThread();
    viBufAddDMA((ViBuf*)(*(int*)0x16120C + 0xD9090));
    return 1;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023D0E0);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023D110);

INCLUDE_ASM("code/_generated/nonmatchings/game/movie/videodec", func_0023D140);
