typedef struct {
    unsigned char pad[0x18];
    unsigned int f18;
    unsigned int f1c;
    unsigned int f20;
    unsigned int f24;
} ProbePauseBar;

extern "C" void probe_callee(int a, int b, int c, int d) asm("func_001FD748");

extern "C" int func_00221930(ProbePauseBar* s) {
    probe_callee(s->f18, s->f18 + s->f20, s->f1c, s->f1c + s->f24);
    return 2;
}
