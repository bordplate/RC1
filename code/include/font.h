#ifndef FONT_H
#define FONT_H

#include "types.h"

// Width-bounded text window descriptor used by the FontPrintWindow API
// (draw_post_post.cpp). 12 shorts = 24 bytes. FontSetWindow fills the
// geometry fields and zeroes the running totals; FontPrintWindow updates
// maxTextH/totalH as it renders.
typedef struct FontWindow {
    short x;
    short y;
    short w;
    short h;
    short textX;
    short textY;
    short maxTextH;
    short totalH;
    short lineH;
    short flags;
    short offX;
    short offY;
} FontWindow;

// Font API entry points (defined in draw_post_post.cpp, C linkage).
extern "C" void FontSetWindow(FontWindow* f, short x, short y, short w, short h,
                              short textX, short textY, short lineH, int flags);
extern "C" void FontPrintWindowMedium(FontWindow* f, long color, u8* text, int length);

#endif
