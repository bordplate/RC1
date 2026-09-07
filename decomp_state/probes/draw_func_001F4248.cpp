// func_001F4248 probe A: constant-based guard address.
// Result: 14 words (goal 13). Load form correct (lui v0; lw v0; addiu sp
// first) but sq ra stays in the prologue and the epilogue lq ra is pulled
// into the bnez delay slot.
extern "C" void func_001FB368();
extern "C" void DrawDebugProfiler();
extern "C" int D_0015F434;

extern "C" void func_001F4248() {
    if (*(int*)0x15F618 == 0) {
        func_001FB368();
        D_0015F434 = 0x7F;
        DrawDebugProfiler();
    }
}
