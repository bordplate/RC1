extern "C" int CamPostUpdRoutineCnt __attribute__((section(".data")));
typedef void (*CamPostUpdFunc)(void);

extern "C" void ExecuteCamPostUpdFuncs(void) {
    int i;
    for (i = 0; i < CamPostUpdRoutineCnt; i++)
        ((CamPostUpdFunc *)0x1892B0)[i]();
    CamPostUpdRoutineCnt = 0;
}
