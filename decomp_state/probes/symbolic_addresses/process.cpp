// Isolated experiment; SDK and handwritten assembly entry points use C ABI.
extern "C" void FlushCache(int);
extern "C" void FastMemCopy(void*, const void*, int);
extern "C" void MobyAnimProc(int*, int*);
extern unsigned char D_00165500[];
extern int* D_0015F638;
extern int* D_0015F63C;

void ProcessMobyAnimData() {
    FlushCache(0);
    // PS2 scratchpad destination: hardware address, not a linked RAM global.
    FastMemCopy((void*)0x70003800, D_00165500, 0x800);
    MobyAnimProc(D_0015F638, D_0015F63C);
}
