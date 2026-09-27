#include "common.h"
#include "types.h"
#include "camera.h"
#include "video.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F2260);

// Particle clip distance per fog state (PartClipDist in Deadlocked).
#define WATER_PART_CLIP_DIST 0x40000
#define LEVEL_PART_CLIP_DIST 0x1F4000

// Underwater fog settings; the on-land source is the levelFog* block below.
extern u8 waterFogR;
extern u8 waterFogG;
extern u8 waterFogB;
extern float waterFogNearDist;
extern float waterFogFarIntensity;
extern float waterFogNearIntensity;
extern float waterFogFarDist;

// Per-level fog settings loaded by the level loader.
extern u8 levelFogR;
extern u8 levelFogG;
extern u8 levelFogB;
extern float levelFogNearDist;
extern float levelFogFarIntensity;
extern float levelFogNearIntensity;
extern float levelFogFarDist;

extern int levelFogMode;
extern int partClipDist;
// GPREL access alias of partClipDist: the underwater branch stores it as a
// GPREL16 in the branch delay slot. ps2eeas expands the bare pseudo as GPREL
// only once this .extern declaration precedes the reference; the plain symbol
// stays self-based absolute for the level branch.
extern int partClipDistGp;
asm(".extern partClipDistGp, 4");

void UpdateViewContext(void);

void UpdateFog(int cameraIndex) {
    // EGC's default COP1 allocation in this branch context does not match the
    // original's float registers, so the four fog floats are pinned per branch.
    if (currentCamera.camUnderWater != 0) {
        ViewCtx* ctx = &viewCtx;
        u8 r = waterFogR;
        u8 g = waterFogG;
        u8 b = waterFogB;
        register float nearDist asm("$f1") = waterFogNearDist;
        register float farIntensity asm("$f2") = waterFogFarIntensity;
        register float nearIntensity asm("$f3") = waterFogNearIntensity;
        register float farDist asm("$f0") = waterFogFarDist;
        ctx->fogR = r;
        ctx->fogG = g;
        ctx->fogB = b;
        ctx->fogNearDist = nearDist;
        ctx->fogFarIntensity = farIntensity;
        ctx->fogNearIntensity = nearIntensity;
        ctx->fogFarDist = farDist;
        partClipDistGp = WATER_PART_CLIP_DIST;
    } else {
        ViewCtx* ctx = &viewCtx;
        u8 r = levelFogR;
        u8 g = levelFogG;
        u8 b = levelFogB;
        register float nearDist asm("$f3") = levelFogNearDist;
        register float farIntensity asm("$f1") = levelFogFarIntensity;
        register float nearIntensity asm("$f2") = levelFogNearIntensity;
        register float farDist asm("$f0") = levelFogFarDist;
        ctx->fogR = r;
        ctx->fogG = g;
        ctx->fogB = b;
        ctx->fogNearDist = nearDist;
        ctx->fogFarIntensity = farIntensity;
        ctx->fogNearIntensity = nearIntensity;
        ctx->fogFarDist = farDist;
        partClipDist = LEVEL_PART_CLIP_DIST;
    }
    UpdateViewContext();
    levelFogMode = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", ParseOcclGrid);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", GetOcclGridFromPair__Fiiiiiif);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", BuildOcclVisibility__Fv);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", InitViewContext__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", UpdateViewContext__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F33B8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", SetPalMode__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", ResetDrawGlobals);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", ResetGsRegisters__Fv);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", DrawDebugProfiler);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F4248);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", SetupGifPaging__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", DoGifPaging__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", GetEffectTex__Fii);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F4650);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F46C8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F4740);

// Second per-draw-phase callback list (AddDrawCallback is the first). The
// unmangled label is the Splat placeholder for this stripped-ELF symbol.
extern u32 drawCallback2Funcs[];
extern u32 drawCallback2Args[];
extern int drawCallback2Count;

void AddDrawCallback2(u32 func, u32 arg) asm("func_001F47B8");
void AddDrawCallback2(u32 func, u32 arg) {
    int idx = drawCallback2Count;
    if (idx < DRAW_CALLBACK_MAX) {
        drawCallback2Funcs[idx] = func;
        drawCallback2Args[idx] = arg;
        drawCallback2Count = idx + 1;
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F4808);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F4880);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", FadeToBlack__FiUi);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F4BE0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F4D98);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F4FB8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F5138);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F5210);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", DrawRectOverlay_FiiiiUl);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F5448);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", DrawTexturedQuad);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F55D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F5808);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F5AB0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F5F10);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", DrawUIFrame);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F6060);

extern int drawOcclusionEnabled;

void enableOcclusion(void) {
    drawOcclusionEnabled = 1;
}

void disableOcclusion(void) {
    drawOcclusionEnabled = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F6200);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", FontPrint);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", FontPrintLarge);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", FontPrintSmall);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F6630);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F6638);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F6928);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F69D0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F6A60);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", FontPrintCenter);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", FontPrintCenterSmall);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", FontPrintCenterLarge);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F6CB8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F6FD0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F7070);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", FontPrintWindow);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F7580);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F75F0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F7660);

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

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F76A0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F7888);

void PutDrawBufferLarge();
void InitViewContext();
void UpdateViewContext();

void draw_prepareFrame(void) {
    PutDrawBufferLarge();
    InitViewContext();
    UpdateViewContext();
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F79A8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F7A30);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F7A88);
