#include "common.h"
#include "types.h"

// The EE maps the VU0 microprogram region at 0x10000000; the game stores a
// tick value at offset 0x800 that vsync_callback folds into the frame clock.
#define VU0_TICK_VALUE_ADDR (0x10000800U)

// Frame clock accumulated by vsync_callback into D_0015ED50; the boot image
// never writes it, so its writer is unconfirmed.
extern s64 frm_clock_time __attribute__((section(".data")));
// Vertical sync counter, incremented by vsync_callback on each sync.
extern s64 frm_vsync_cnt __attribute__((section(".data")));
// Written by vsync_callback on every vertical sync; never read in the boot
// image, so its consumer (likely a level overlay) is unconfirmed.
extern s64 D_0015ED50;

int vsync_callback(int) {
    frm_vsync_cnt += 1;
    // Volatile constant-address cast required: a named reference to
    // 0x10000800 compiles to a fused lui/lw load that does not match the
    // original li + load + zero-extend sequence.
    D_0015ED50 = frm_clock_time + *(volatile unsigned int*)VU0_TICK_VALUE_ADDR;
    return 0;
}
