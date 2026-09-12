typedef unsigned char u8;

extern "C" void PatchTieGifs(void);
extern "C" void FastMemCopy(void* p1, void* p2, int p3);

extern u8 D_001E2A00[];
extern u8 D_001E3000[];
extern u8 D_001E3E00[];
extern u8 D_001E4200[];

extern "C" void func_00235840(void) {
    PatchTieGifs();
    FastMemCopy(D_001E3000, D_001E4200, 0x200);
    FastMemCopy(D_001E2A00, D_001E3E00, 0x400);
    PatchTieGifs();
}
