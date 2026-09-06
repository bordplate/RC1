extern unsigned char D_0013D3D8 __attribute__((section(".data")));

extern "C" int menu_threshold(int x, float unused1, float unused2, float y) {
    if (x >= 190)
        return D_0013D3D8 != 0;
    float threshold = 58.5f;
    asm volatile("nop" : : "f"(threshold));
    return threshold <= y;
}
