extern "C" volatile int CamPostUpdRoutineCnt __attribute__((section(".data")));
typedef void (*CamPostUpdFunc)(void);
extern "C" CamPostUpdFunc CamPostUpdRoutines[];

extern "C" void ExecuteCamPostUpdFuncs(void) {
    int i;
    for (i = 0; i < CamPostUpdRoutineCnt; i++)
        CamPostUpdRoutines[i]();
    CamPostUpdRoutineCnt = 0;
}
