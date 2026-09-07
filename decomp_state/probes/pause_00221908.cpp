extern "C" int D_0013CB04 __attribute__((section(".data")));
extern "C" int* D_001D5BF8 __attribute__((section(".data")));
extern "C" int D_001D22F8 __attribute__((section(".data")));

extern "C" int func_00221908(void) {
    if (D_0013CB04 & 0x40) {
        D_001D5BF8 = &D_001D22F8;
    }
    return 0;
}
