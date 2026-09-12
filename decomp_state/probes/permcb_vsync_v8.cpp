#include "types.h"

extern "C" s64 frm_vsync_cnt __attribute__((section(".data")));
extern "C" s64 frm_clock_time __attribute__((section(".data")));
extern "C" s64 D_0015ED50;

int vsync_callback(int numLines) {
    frm_vsync_cnt += 1;
    D_0015ED50 = frm_clock_time + *(volatile unsigned int*)0x10000800;
    return 0;
}
