extern "C" int CamPostUpdRoutineCnt __attribute__((section(".data")));
typedef void (*CamPostUpdFunc)(void);

extern "C" void ExecuteCamPostUpdFuncs(void) {
    int i = 0;
    if (CamPostUpdRoutineCnt > 0) {
        register volatile unsigned routinesHi asm("$2") = 0x190000;
        CamPostUpdFunc *routine = (CamPostUpdFunc *)(routinesHi - 0x6d50);
        do {
            (*routine++)();
            ++i;
        } while (i < CamPostUpdRoutineCnt);
    }
    CamPostUpdRoutineCnt = 0;
}
