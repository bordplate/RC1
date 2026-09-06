extern "C" int D_00160F00;

void VU1_addDataRef(void* data, int count) {
    *(int*)(*(int*)0x160F00 + 0) = count | 0x30000000;
    *(int*)(*(int*)0x160F00 + 4) = (int)data;
    *(int*)(*(int*)0x160F00 + 8) = 0;
    *(int*)(*(int*)0x160F00 + 12) = 0;
    int next = *(int*)0x160F00 + 16;
    D_00160F00 = next;
}
