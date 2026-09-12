#include "types.h"

extern "C" s64 frm_vsync_cnt __attribute__((section(".data")));
extern "C" s64 frm_clock_time __attribute__((section(".data")));
extern "C" s64 D_0015ED50;
extern "C" volatile u32 D_0010000800 __attribute__((section(".data")));

int vsync_callback(int numLines) {
    frm_vsync_cnt += 1;
    D_0015ED50 = frm_clock_time + D_0010000800;
    return 0;
}
