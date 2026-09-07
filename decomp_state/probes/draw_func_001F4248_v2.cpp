// func_001F4248 probe B: symbol-based guard address (section .lit4).
// Result: 13 words with the goal schedule (sq ra in bnez ds, no redundant
// lq) BUT the lui is hoisted above `addiu sp` and the load value lands in
// v1 (lw v1 / bnez v1) instead of v0. Plain in-window extern instead gives
// a single gp-relative lw (12 words).
extern "C" void func_001FB368();
extern "C" void DrawDebugProfiler();
extern "C" int D_0015F434;
extern "C" int D_0015F618 __attribute__((section(".lit4")));

extern "C" void func_001F4248() {
    if (D_0015F618 == 0) {
        func_001FB368();
        D_0015F434 = 0x7F;
        DrawDebugProfiler();
    }
}
