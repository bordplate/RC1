extern "C" int D_00160F00 __attribute__((section(".data")));

void VU1_addDataRef(void* data, int count) {
    *(int*)(D_00160F00 + 0) = count | 0x30000000;
    *(int*)(D_00160F00 + 4) = (int)data;
    *(int*)(D_00160F00 + 8) = 0;
    *(int*)(D_00160F00 + 12) = 0;
    D_00160F00 += 16;
}
