extern unsigned char menuItemEnabled_68 __attribute__((section(".data")));

extern "C" int menu_threshold(int x, float unused1, float unused2, float y) {
    if (x >= 190)
        return menuItemEnabled_68 != 0;
    float threshold = 58.5f;
    asm volatile("nop" : : "f"(threshold));
    return threshold <= y;
}
