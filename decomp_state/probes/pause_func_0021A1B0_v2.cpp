typedef struct {
    int pad[13];
    int f34;
} Obj;

extern "C" int D_0015EE90 __attribute__((section(".data")));
extern "C" int D_001D4810[];
extern "C" int D_001D4840[];

extern "C" int func_0021A1B0(Obj *self) {
    if (D_0015EE90 == 0)
        self->f34 = (int)D_001D4840;
    else
        self->f34 = (int)D_001D4810;
    return 0;
}
