// Isolated code-generation experiment; names/ABI follow mobyfunc.cpp.
// Link addresses are supplied by the harness, never by these expressions.
extern int D_0018A2D8[];
extern int D_0015FF14;
extern int D_0015FF18;
extern int vu1ChainHead;
extern int D_00160F08;
extern char vuChainOverflowMessage[];
void DrawMobysSetup();
void InitMobyClassDists();
extern "C" void DrawMobysCleanUp();
extern "C" int MobyProc(int, int, int, int);
extern "C" void STUB_printf(const char*, ...);

extern "C" void DrawMobys() {
    DrawMobysSetup();
    if (D_0018A2D8[0] != 0) {
        InitMobyClassDists();
        int ret = MobyProc(D_0015FF18, D_0015FF14, -1, 1);
        D_0015FF14 = ret;
        if (vu1ChainHead > D_00160F08) {
            STUB_printf(vuChainOverflowMessage);
        }
    }
    DrawMobysCleanUp();
}
