typedef struct {
    int pad[13];
    int f34;
} Obj;

extern "C" int func_0021A1B0(Obj *self) {
    int val;
    if (*(int *)0x15EE90 == 0)
        val = 0x1D4840;
    else
        val = 0x1D4810;
    self->f34 = val;
    return 0;
}
