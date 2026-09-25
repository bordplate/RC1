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

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F0CE0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F2068);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F21A8);

void draw_noopA(void) {
}
void draw_noopB(void) {
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F21C0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F2260);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", UpdateFog__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", ParseOcclGrid);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", GetOcclGridFromPair__Fiiiiiif);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", BuildOcclVisibility__Fv);

void BuildOcclVisibility(void);

extern "C" int OcclUpdate __attribute__((section(".data")));
extern "C" char OcclVisibility[] __attribute__((section(".data")));

void UpdateOcclusion() {
    if (OcclUpdate == 0) {
        FastMemSet(OcclVisibility, -1, 0x80);
    } else if (OcclUpdate == 2) {
        BuildOcclVisibility();
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", InitViewContext__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", UpdateViewContext__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F33B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", SetPalMode__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", ResetDrawGlobals);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", ResetGsRegisters__Fv);

// The EE maps the GS (Graphics Synthesizer) register window at 0x12000000;
// these are the display-control registers ResetGsRegistersPr resets.
#define GS_PMODE 0x12000000
#define GS_SMODE2 0x12000020
#define GS_DISPFB1 0x12000070
#define GS_DISPLAY1 0x12000080
#define GS_DISPFB2 0x12000090
#define GS_DISPLAY2 0x120000A0
#define GS_EXTWRITE 0x120000D0
#define GS_BGCOLOR 0x120000E0

// PMODE reset value: fixed-point 255, alpha and color shading enabled,
// polygon (PT0) primitives.
#define GS_PMODE_POLY 0xFFA1

struct GsDisplaySettings {
    u64 smode2;
    u64 dispfb;
    u64 display;
};

// Zero-initialized GS display settings read by ResetGsRegistersPr; the boot
// image never writes them, so their writer (likely level code) is unconfirmed.
extern struct GsDisplaySettings gsDisplaySettings;

void ResetGsRegistersPr() {
    // Volatile constant-address casts: plain casts let EGC fold the stores to
    // a shared 0x1200 base register, which the original does not do.
    *(volatile u64*)GS_BGCOLOR = 0;
    *(volatile u64*)GS_PMODE = GS_PMODE_POLY;
    *(volatile u64*)GS_SMODE2 = gsDisplaySettings.smode2;
    *(volatile u64*)GS_DISPFB1 = gsDisplaySettings.dispfb;
    *(volatile u64*)GS_DISPFB2 = gsDisplaySettings.dispfb;
    *(volatile u64*)GS_DISPLAY1 = gsDisplaySettings.display;
    *(volatile u64*)GS_DISPLAY2 = gsDisplaySettings.display;
    *(volatile u64*)GS_EXTWRITE = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", DrawDebugProfiler);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F4248);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", SetupGifPaging__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", DoGifPaging__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", GetEffectTex__Fii);

// Each per-draw-phase callback list holds up to this many (func, arg) pairs.
#define DRAW_CALLBACK_MAX 0x40

// drawCallbackCount (0x15F464) is one of the per-draw-phase callback-list
// counters zeroed by ResetDrawGlobals. It is in the GP window, so a
// constant-address cast is required to reproduce the original's self-based
// absolute access under this TU's GNU assembler (a plain extern emits one
// GPREL16, a .data symbol a two-register load). The named extern matches under
// the SN assembler, but draw.o must stay a GNU-compat TU; see
// decomp_state/notes/draw_AddDrawCallback__FUiUi.md. The symbol is kept in
// symbols.txt for the sibling functions' generated assembly.
extern u32 drawCallbackFuncs[];
extern u32 drawCallbackArgs[];

void AddDrawCallback(u32 func, u32 arg) {
    int idx = *(int*)0x15F464;
    if (idx < DRAW_CALLBACK_MAX) {
        drawCallbackFuncs[idx] = func;
        drawCallbackArgs[idx] = arg;
        *(int*)0x15F464 = idx + 1;
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F4650);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F46C8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F4740);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F47B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F4808);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F4880);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", FadeToBlack__FiUi);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F4BE0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F4D98);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F4FB8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F5138);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F5210);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", DrawRectOverlay_FiiiiUl);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F5448);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", DrawTexturedQuad);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F55D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F5808);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F5AB0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F5F10);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", DrawUIFrame);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F6060);

extern int drawOcclusionEnabled;

void enableOcclusion(void) {
    drawOcclusionEnabled = 1;
}

void disableOcclusion(void) {
    drawOcclusionEnabled = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F6200);

// C linkage: this still-assembly font helper at 0x001F6200 sums glyph widths;
// the original call target is an unmangled entry point.
extern "C" int fontMeasureString(char* text, int length, char* glyphs);

extern char fontSmallGlyphs[];

int drawTextSmall(char* text, int length) {
    return fontMeasureString(text, length, fontSmallGlyphs);
}

extern char fontMediumGlyphs[];

int drawTextMedium(char* text, int length) {
    return fontMeasureString(text, length, fontMediumGlyphs);
}

extern char fontLargeGlyphs[];

int drawTextLarge(char* text, int length) {
    return fontMeasureString(text, length, fontLargeGlyphs);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", FontPrint);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", FontPrintLarge);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", FontPrintSmall);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F6630);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F6638);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F6928);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F69D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F6A60);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", FontPrintCenter);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", FontPrintCenterSmall);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", FontPrintCenterLarge);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F6CB8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F6FD0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F7070);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", FontPrintWindow);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F7580);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F75F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F7660);

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

// C linkage: this font API entry point retains the original unmangled name.
extern "C" void FontSetWindow(FontWindow* f, short x, short y, short w, short h,
                              short textX, short textY, short lineH, int flags) {
    f->x = x;
    f->y = y;
    f->w = w;
    f->h = h;
    f->textX = textX;
    f->textY = textY;
    f->lineH = lineH;
    f->flags = flags;
    f->maxTextH = 0;
    f->totalH = 0;
    f->offX = 0;
    f->offY = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F76A0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F7888);

void PutDrawBufferLarge();
void InitViewContext();
void UpdateViewContext();

void draw_prepareFrame(void) {
    PutDrawBufferLarge();
    InitViewContext();
    UpdateViewContext();
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F79A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F7A30);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F7A88);
