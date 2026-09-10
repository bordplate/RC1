#include "common.h"

extern int drawTextureDmaState[20] __attribute__((section(".data")));

// Symbol override: this recovered helper occupies an address-based generated
// entry point whose original label is not naturally cfront-mangled.
void draw_resetTextureDmaState(void) asm("func_001F0B88");

void draw_resetTextureDmaState(void) {
    int value = 1;
    int* state = drawTextureDmaState;
    int count = 19;
    state += count;
    do {
        *state = value;
        count--;
        state--;
    } while (count >= 0);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F0BC8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F0BD0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F0C48);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F0CE0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F2068);

struct Camera {
    float f00;
    char pad_04[0x3C];
    float matrix[16];
    char pad_80[0xC0];
    float f140;
    float f144;
    float f148;
    char pad_14C[0x64];
    float f1B0;
};

extern Camera currentCamera;
extern Camera drawCamera;

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
    v[12] = -currentCamera.f140 * 1024.0f;
    v[13] = -currentCamera.f144 * 1024.0f;
    v[14] = -currentCamera.f148 * 1024.0f;
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

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", ResetGsRegistersPr__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", DrawDebugProfiler);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F4248);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", SetupGifPaging__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", DoGifPaging__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", GetEffectTex__Fii);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw", func_001F4600);

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
