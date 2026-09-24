// Probe for func_001ED7F0: per-frame camera line-collision check.
// v16: v14 (128-bit, combined loop, FP pins) + tied GPR barriers before each
//   CollOutput.point use, to defeat the loop-invariant GPR base hoisting
//   (original re-materializes lui 0x19 per path, never holds it in a callee-saved).

typedef unsigned int CamQuad __attribute__((mode(TI)));

union CamPt {
    CamQuad q;
    struct { float x, y, z, w; } v;
};

struct UpdateCam {
    char pad_00[0x86];
    short collMode;
};

struct Cam {
    char pad_00[0x140];
    float posX, posY, posZ, posW;   // 0x140..0x14F (16-byte Vec4)
    char pad_150[0x30];
    unsigned int pCur;              // 0x180
    char pad_184[0x210];
    unsigned int collValue;         // 0x394
};

struct CollOut {
    float pad[8];                   // 0x00..0x1F
    float point[4];                 // 0x20..0x2F (16-byte Vec4)
};

extern Cam currentCamera;
extern int camCollStaged;
extern CollOut CollOutput;
extern "C" int CollLine_Fix(void*, void*, int, void*, void*);
int func_001F0B58(void) asm("func_001F0B58");
float func_002135F0(float*, void*) asm("func_002135F0");

void Camera_checkCollLine(void) {
    CamPt a, b;
    UpdateCam* pCur = (UpdateCam*)currentCamera.pCur;
    if (pCur->collMode == 6 || camCollStaged != 0) {
        currentCamera.collValue = 0;
        return;
    }
    volatile CamQuad* pQ = (volatile CamQuad*)&currentCamera.posX;
    a.q = *pQ;
    b.q = *pQ;
    a.v.z += 0.75f;
    b.v.z -= 0.75f;
    int count = 0;
    while (count < 6 && CollLine_Fix(&a, &b, 18, 0, 0) != 0) {
        if (func_001F0B58() == 0) {
            float* pf = CollOutput.point;
            asm volatile("" : "+r"(pf));
            float r3 = func_002135F0(pf, 0);
            register float tolerance asm("$f1") = 0.04f;
            asm volatile("" : "+f"(tolerance));
            if (currentCamera.posZ < r3 + tolerance)
                currentCamera.collValue = 1;
            else
                currentCamera.collValue = 0;
            return;
        }
        CamQuad* pc = (CamQuad*)CollOutput.point;
        asm volatile("" : "+r"(pc));
        a.q = *pc;
        register float retreat asm("$f0") = 0.01f;
        asm volatile("" : "+f"(retreat));
        a.v.z -= retreat;
        count++;
    }
}
