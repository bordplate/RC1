typedef void (*CamPostUpdFunc)(void);
extern "C" CamPostUpdFunc CamPostUpdRoutines[];

extern "C" void ExecuteCamPostUpdFuncs(void) {
    int i;
    for (i = 0; i < *(int*)0x15EF8C; i++)
        CamPostUpdRoutines[i]();
    *(int*)0x15EF8C = 0;
}
