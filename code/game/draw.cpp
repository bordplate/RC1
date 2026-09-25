#include "common.h"
#include "types.h"
#include "camera.h"

#define DRAW_TEXTURE_DMA_STATE_COUNT 20
#define DRAW_TEXTURE_DMA_STATE_INITIAL 1

extern int drawTextureDmaState[DRAW_TEXTURE_DMA_STATE_COUNT]
    __attribute__((section(".data")));

void draw_resetTextureDmaState(void) {
    int value = DRAW_TEXTURE_DMA_STATE_INITIAL;
    int* state = drawTextureDmaState;
    int count = DRAW_TEXTURE_DMA_STATE_COUNT - 1;
    state += count;
    do {
        *state = value;
        count--;
        state--;
    } while (count >= 0);
}

// Unreachable dead tail the original compiler emitted after
// draw_resetTextureDmaState: a 0x2A0 stack deallocation matching no frame,
// plus the alignment nop before func_001F0BD0. EGC 2.95.2 never regenerates
// a dead frame deallocation after the epilogue (probed; see
// decomp_state/notes/989snd_func_0012DF18.md), so the bytes are preserved
// with raw asm. The .align 3 and the nonmatching/glabel pair reproduce the
// generated assembly's layout (one alignment nop before the fragment).
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001F0BC8, 0x4\n"
    "glabel func_001F0BC8\n"
    "    .word 0x27bd02a0\n"
    "endlabel func_001F0BC8\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

// Font text buffer state: fontTextCursor points into the 2KB font data
// buffer at 0x18A300 and advances past each submitted text;
// fontTextSlotIndex selects the next fontTextSlots request entry;
// fontTextFormat holds the "%s" format passed to sprintf.
typedef struct {
    int x;
    int y;
    int field_0x08;
    char* textStart;
} FontTextSlot;

// The slot count is not provable from the boot ELF: the array sits in a
// run of zero data with no visible bound, and fontTextSlotIndex is
// incremented without wrap. 20 matches the drawTextureDmaState count just
// before it; the size only appears in this declaration and affects no
// codegen (EGC emits no bounds checks).
#define FONT_TEXT_SLOT_COUNT 20

extern FontTextSlot fontTextSlots[FONT_TEXT_SLOT_COUNT]
    __attribute__((section(".data")));
extern char* fontTextCursor;
extern int fontTextSlotIndex;
extern const char fontTextFormat[];

// Per-character glyph widths for the font: entry i holds the advance width
// of character i + 0x20, so text is measured as fontCharWidths[c - 0x20].
// Only indices 0x00-0x5F are read (wider characters clamp to 0x20).
extern int fontCharWidths[];

// C linkage: 0x116248 is the SDK sprintf, generated code
// (code/_generated/glibc.s) with an unmangled entry point.
extern "C" int sprintf(char* str, const char* format, ...);

// Records a font text draw request in the next fontTextSlots entry (the
// position x/y, the unknown field_0x08, and the current fontTextCursor as
// textStart), then appends the sprintf-formatted text to the font data
// buffer at fontTextCursor and advances the cursor past it, including the
// NUL terminator.
//
// Symbol override: the generated caller still references the Splat
// placeholder name for this entry point.
void fontTextSubmit(int x, int y, int field_0x08, char* text)
    asm("func_001F0BD0");

void fontTextSubmit(int x, int y, int field_0x08, char* text) {
    int index = fontTextSlotIndex;

    fontTextSlots[index].x = x;
    fontTextSlots[index].y = y;
    fontTextSlots[index].field_0x08 = field_0x08;
    fontTextSlots[index].textStart = fontTextCursor;
    fontTextSlotIndex = index + 1;

    int count = sprintf(fontTextCursor, fontTextFormat, text) + 1;
    fontTextCursor += count;
}

// Unreachable dead tail the original compiler emitted after fontTextSubmit:
// a 0x30 stack deallocation matching no frame (fontTextSubmit's own 0x10
// epilogue is the addiu sp,sp,16 in its jr delay slot at 0x1F0C40), plus the
// alignment nop before func_001F0C50. EGC 2.95.2 never regenerates a dead
// frame deallocation after the epilogue, so the bytes are preserved with raw
// asm. The .align 3 reproduces the alignment nop at 0x1F0C44.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001F0C48, 0x8\n"
    "glabel func_001F0C48\n"
    "    .word 0x27bd0030\n"
    "    .word 0x00000000\n"
    "endlabel func_001F0C48\n"
    "    .set reorder\n"
    "    .set at\n"
);

// Measures the pixel width of a font string (sum of fontCharWidths entries,
// indexed by c - 0x20 with c >= 0x60 clamped to index 0x20), then submits it
// via fontTextSubmit centered on x: the text is drawn at x - width/2, and
// that centered left edge is returned. The real code starts at 0x1F0C50,
// after the dead tail above.
int fontTextSubmitCentered(int x, int y, int field_0x08, unsigned char* text)
    asm("func_001F0C50");

int fontTextSubmitCentered(int x, int y, int field_0x08, unsigned char* text) {
    int sum = 0;
    unsigned char* p = text;
    if (*text) {
        do {
            unsigned char c = *p++ - 0x20;
            int w = c;
            if (c >= 0x60)
                w = 0x20;
            sum += fontCharWidths[w];
        } while (*p);
    }
    x -= sum >> 1;
    fontTextSubmit(x, y, field_0x08, (char*)text);
    return x;
}
