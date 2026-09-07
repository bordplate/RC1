extern "C" int func_001233F0(void);
extern char D_001E8360[];
extern "C" void STUB_printf(const char* fmt, ...);

extern "C" void memcard_Init(void) {
    if (func_001233F0() != 0)
        STUB_printf(D_001E8360);
}
