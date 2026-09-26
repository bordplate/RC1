#include "common.h"
#include "types.h"
#include "camera.h"
#include "video.h"

// C linkage: these helpers are handwritten VU assembly in the generated
// fast-function region, so their entry points are not cfront-mangled.
extern "C" void draw_loadViewMatrix(void* a0);
extern "C" void draw_transformMatrix(void* a0, void* a1, void* a2);
extern "C" void draw_scaleVector(void* a0, void* a1, float f);
extern "C" void draw_transformVector(void* a0, void* a1, void* a2);

void projectWorldPoint(float* out, float* vec) {
    float v[16];
    float m[16];
    float s4[4];
    float r4[4];
    draw_loadViewMatrix(v);
    v[12] = -currentCamera.pos * 1024.0f;
    v[13] = -currentCamera.posY * 1024.0f;
    v[14] = -currentCamera.posZ * 1024.0f;
    draw_transformMatrix(m, currentCamera.matrix, v);
    draw_scaleVector(s4, vec, 1024.0f);
    s4[3] = 1.0f;
    draw_transformVector(r4, s4, m);
    float scale = drawCamera.f00 / r4[3];
    out[2] = r4[2] * 0.0009765625f;
    r4[0] = r4[0] * scale + 2048.0f;
    r4[1] = r4[1] * scale + 2048.0f;
    out[0] = r4[0] * 16.0f;
    out[1] = r4[1] * 16.0f;
}

// Unreachable dead tail the original compiler emitted after
// projectWorldPoint: a store of zero to 0x1B0 of the live v0, plus the
// alignment nop before draw_noopA. EGC 2.95.2 never regenerates a dead store
// after the epilogue (probed; see decomp_state/notes/989snd_func_0012DF18.md),
// so the bytes are preserved with raw asm. The .align 3 and the
// nonmatching/glabel pair reproduce the generated assembly's layout (one
// alignment nop before the fragment).
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001F21A8, 0x4\n"
    "glabel func_001F21A8\n"
    "    .word 0xAC4001B0\n"
    "endlabel func_001F21A8\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

void draw_noopA(void) {
}
void draw_noopB(void) {
}
