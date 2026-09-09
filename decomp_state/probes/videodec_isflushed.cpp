typedef struct VideoDec {
    unsigned char _pad[0x50];
} VideoDec;

int videoDecInputCount(VideoDec* self);

extern "C" unsigned int func_0012BA58(VideoDec* self);

int videoDecIsFlushed(VideoDec* self) {
    int ret = 0;
    if (videoDecInputCount(self) == 0)
        ret = func_0012BA58(self) > 0;
    return ret;
}
