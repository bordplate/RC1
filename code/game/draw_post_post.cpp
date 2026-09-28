#include "common.h"
#include "types.h"
#include "camera.h"
#include "actuator.h"
#include "video.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F2260);

// Particle clip distance per fog state (PartClipDist in Deadlocked).
#define WATER_PART_CLIP_DIST 0x40000
#define LEVEL_PART_CLIP_DIST 0x1F4000

// Underwater fog settings; the on-land source is the levelFog* block below.
// Distances are stored in 1/1024 units; intensities are 0-255 (255 = clear).
extern u8 waterFogR;
extern u8 waterFogG;
extern u8 waterFogB;
extern float waterFogNearDist;
extern float waterFogFarDist;
extern float waterFogNearIntensity;
extern float waterFogFarIntensity;

// Per-level fog settings loaded by the level loader.
extern u8 levelFogR;
extern u8 levelFogG;
extern u8 levelFogB;
extern float levelFogNearDist;
extern float levelFogFarDist;
extern float levelFogNearIntensity;
extern float levelFogFarIntensity;

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
        register float farDist asm("$f2") = waterFogFarDist;
        register float nearIntensity asm("$f3") = waterFogNearIntensity;
        register float farIntensity asm("$f0") = waterFogFarIntensity;
        ctx->fogR = r;
        ctx->fogG = g;
        ctx->fogB = b;
        ctx->fogNearDist = nearDist;
        ctx->fogFarDist = farDist;
        ctx->fogNearIntensity = nearIntensity;
        ctx->fogFarIntensity = farIntensity;
        partClipDistGp = WATER_PART_CLIP_DIST;
    } else {
        ViewCtx* ctx = &viewCtx;
        u8 r = levelFogR;
        u8 g = levelFogG;
        u8 b = levelFogB;
        register float nearDist asm("$f3") = levelFogNearDist;
        register float farDist asm("$f1") = levelFogFarDist;
        register float nearIntensity asm("$f2") = levelFogNearIntensity;
        register float farIntensity asm("$f0") = levelFogFarIntensity;
        ctx->fogR = r;
        ctx->fogG = g;
        ctx->fogB = b;
        ctx->fogNearDist = nearDist;
        ctx->fogFarDist = farDist;
        ctx->fogNearIntensity = nearIntensity;
        ctx->fogFarIntensity = farIntensity;
        partClipDist = LEVEL_PART_CLIP_DIST;
    }
    UpdateViewContext();
    levelFogMode = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", ParseOcclGrid);

// C linkage: ParseOcclGrid is defined by the generated INCLUDE_ASM fallback
// as the unmangled symbol at 0x1f2690; GetOcclGridFromPair calls that entry.
// Returns the matched grid cell (pgrid + (child<<7)) or 0 when not found.
extern "C" char* ParseOcclGrid(int x, int y, int z);

// fraction is the caller-computed fractional part of the scaled occlusion
// coordinate. Below this midpoint the first cell is parsed first; at or above
// it the second cell is.
#define OCCL_CELL_FRACTION_MIDPOINT 0.5f

void GetOcclGridFromPair(int cell0X, int cell0Y, int cell0Z,
                         int cell1X, int cell1Y, int cell1Z,
                         float fraction) {
    int secondX, secondY, secondZ;

    if (fraction < OCCL_CELL_FRACTION_MIDPOINT) {
        if (ParseOcclGrid(cell0X, cell0Y, cell0Z) != 0)
            return;
        secondX = cell1X;
        secondY = cell1Y;
        secondZ = cell1Z;
    } else {
        if (ParseOcclGrid(cell1X, cell1Y, cell1Z) != 0)
            return;
        secondX = cell0X;
        secondY = cell0Y;
        secondZ = cell0Z;
    }

    ParseOcclGrid(secondX, secondY, secondZ);
}

// OcclMode 0 merges the neighbor grid cells around the camera into
// OcclVisibilityMerged and copies the result into OcclVisibility; the other
// modes rebuild OcclVisibility from the previous grid, the staged camera
// commit state, or a precomputed octant table.
extern "C" int OcclMode;
extern "C" int OcclInvalidGrid;
// GPREL access aliases: the first branch and the OcclMode 0 setup store
// these as GPREL16, while the mode switch reads them back with self-based
// absolute loads. ps2eeas expands a bare pseudo as GPREL only once the
// matching .extern declaration precedes the reference; the plain symbols
// stay self-based absolute.
extern "C" int OcclInvalidGridGp;
asm(".extern OcclInvalidGridGp, 4");
extern "C" char* OcclPreviousGrid;
extern "C" char* OcclPreviousGridGp;
asm(".extern OcclPreviousGridGp, 4");
extern "C" float* OcclOct;
extern "C" u8 OcclVisibility[];
extern "C" u8 OcclVisibilityMerged[];

// GetOcclGridFromPair returns void, but its final ParseOcclGrid call leaves
// the matched cell pointer in v0; the alias reads that leftover value.
extern "C" char* getOcclGridFromPairCell(int cell0X, int cell0Y, int cell0Z,
                                         int cell1X, int cell1Y, int cell1Z,
                                         float fraction) asm("GetOcclGridFromPair__Fiiiiiif");

// FastMemOr16: 128-bit logical OR merge, dst[i] = a[i] | b[i] for size bytes.
void FastMemOr16(void* dst, void* a, void* b, int size);
void FastMemZero16(void* p, int size);

#define OCCL_VISIBILITY_SIZE 0x80
#define OCCL_VISIBILITY_VALID_FLAG 0x80
#define OCCL_CELL_SCALE 0.25f
#define OCCL_OCT_SLOT_STRIDE 0x80
#define OCCL_OCT_SLOT_DATA_OFFSET 0x10

void BuildOcclVisibility(void) {
    int cellX, cellY, cellZ;
    char* mergedCell;
    char* pairX;
    char* pairY;
    char* pairZ;
    float* oct;
    int octX, octY, octZ;
    int octIndex;
    // staged must be read from the occlCamState field, not the scalar
    // occlCamStaged (a 4-byte scalar is small data under -G8 and expands to
    // a self-based lui/lw pair), and pinned to v1: the original keeps the
    // 0x190000 hi page live in v0 (shared with the same-page OcclVisibility
    // destinations), so the load is `lui v0 / lw v1`, not self-based.
    register int staged asm("$3");

    cellX = func_001FA6D0(currentCamera.pos * OCCL_CELL_SCALE);
    cellY = func_001FA6D0(currentCamera.posY * OCCL_CELL_SCALE);
    cellZ = func_001FA6D0(currentCamera.posZ * OCCL_CELL_SCALE);
    mergedCell = ParseOcclGrid(cellX, cellY, cellZ);
    if (mergedCell != 0) {
        OcclInvalidGrid = 0;
        FastMemCopy(OcclVisibility, mergedCell, OCCL_VISIBILITY_SIZE);
        OcclPreviousGridGp = mergedCell;
        goto visibilityDone;
    }
    OcclInvalidGridGp = 1;
    if (OcclMode == 0) {
        pairX = getOcclGridFromPairCell(cellX - 1, cellY, cellZ, cellX + 1, cellY, cellZ,
                                        currentCamera.pos * OCCL_CELL_SCALE - func_001FA6C0(cellX));
        pairY = getOcclGridFromPairCell(cellX, cellY - 1, cellZ, cellX, cellY + 1, cellZ,
                                        currentCamera.posY * OCCL_CELL_SCALE - func_001FA6C0(cellY));
        pairZ = getOcclGridFromPairCell(cellX, cellY, cellZ - 1, cellX, cellY, cellZ + 1,
                                        currentCamera.posZ * OCCL_CELL_SCALE - func_001FA6C0(cellZ));
        if (pairX != 0 || pairY != 0 || pairZ != 0) {
            FastMemZero16(OcclVisibilityMerged, OCCL_VISIBILITY_SIZE);
            if (pairX != 0)
                FastMemOr16(OcclVisibilityMerged, OcclVisibilityMerged, pairX, OCCL_VISIBILITY_SIZE);
            if (pairY != 0)
                FastMemOr16(OcclVisibilityMerged, OcclVisibilityMerged, pairY, OCCL_VISIBILITY_SIZE);
            if (pairZ != 0)
                FastMemOr16(OcclVisibilityMerged, OcclVisibilityMerged, pairZ, OCCL_VISIBILITY_SIZE);
            mergedCell = (char*)OcclVisibilityMerged;
            OcclPreviousGrid = mergedCell;
            FastMemCopy(OcclVisibility, mergedCell, OCCL_VISIBILITY_SIZE);
        }
    }
    if (mergedCell != 0)
        goto visibilityDone;
    switch (OcclMode) {
    case 0:
        staged = occlCamState.staged;
        if (staged == 0 && OcclPreviousGrid != 0)
            FastMemCopy(OcclVisibility, OcclPreviousGrid, OCCL_VISIBILITY_SIZE);
        else
            FastMemSet(OcclVisibility, -1, OCCL_VISIBILITY_SIZE);
        break;
    case 1:
        FastMemSet(OcclVisibility, -1, OCCL_VISIBILITY_SIZE);
        break;
    case 2:
        if (OcclOct != 0) {
            oct = OcclOct;
            octX = currentCamera.pos - oct[0] > 0.0f;
            octY = currentCamera.posY - oct[1] > 0.0f;
            octZ = currentCamera.posZ - oct[2] > 0.0f;
            octIndex = octZ + (octY << 1) + (octX << 2);
            FastMemCopy(OcclVisibility,
                        (char*)oct + (octIndex * OCCL_OCT_SLOT_STRIDE + OCCL_OCT_SLOT_DATA_OFFSET),
                        OCCL_VISIBILITY_SIZE);
        } else {
            staged = occlCamState.staged;
            if (staged == 0 && OcclPreviousGrid != 0) {
                FastMemCopy(OcclVisibility, OcclPreviousGrid, OCCL_VISIBILITY_SIZE);
            } else {
                FastMemSet(OcclVisibility, -1, OCCL_VISIBILITY_SIZE);
            }
        }
        break;
    }
visibilityDone:
    OcclVisibility[0x7F] |= OCCL_VISIBILITY_VALID_FLAG;
}

extern "C" int OcclUpdate __attribute__((section(".data")));

void UpdateOcclusion() {
    if (OcclUpdate == 0) {
        FastMemSet(OcclVisibility, -1, 0x80);
    } else if (OcclUpdate == 2) {
        BuildOcclVisibility();
    }
}

// The occlusion view rectangle is centered on this fixed-point value
// (pre-scale; InitViewContext multiplies the min/max by 16).
#define OCCL_VIEW_CENTER 0x800

// View-context defaults written by InitViewContext.
// Perspective-box depths of the projection (see ViewCtx.nearClip/farClip).
#define VIEWCTX_NEAR_CLIP 32.0f
#define VIEWCTX_FAR_CLIP 745472.0f
// Default aspect ratio; yratio derives from it per video mode.
#define VIEWCTX_DEFAULT_XRATIO 0.63f
// xpix/ypix are half the draw size; xclip/yclip are 4x the pix values.
#define VIEWCTX_HALF_PIXEL 0.5f
#define VIEWCTX_CLIP_SCALE 4.0f
// Boot fog-ramp defaults: clear (intensity 255) up to a 512-unit far
// distance (stored in 1/1024 units), full fog (intensity 0) at the far end.
#define VIEWCTX_DEFAULT_FOG_FAR_DIST 524288.0f
#define VIEWCTX_DEFAULT_FOG_NEAR_INTENSITY 255.0f

void InitViewContext(void) {
    int rawX = occlCamParamBase.paramX;
    int rawY = occlCamParamBase.paramY;
    // The params are 16-bit fixed-point; sign-extend and keep a halved copy.
    int x = (s16)rawX;
    int y = (s16)rawY;
    int halfX = (s16)rawX >> 1;
    int halfY = (s16)rawY >> 1;

    occlViewParams.paramX = x;
    occlViewParams.paramY = y;
    occlViewParams.halfX = halfX;
    occlViewParams.halfY = halfY;
    occlViewParams.minX = (OCCL_VIEW_CENTER - halfX) << 4;
    occlViewParams.minY = (OCCL_VIEW_CENTER - halfY) << 4;
    occlViewParams.maxX = (halfX + OCCL_VIEW_CENTER) << 4;
    occlViewParams.maxY = (halfY + OCCL_VIEW_CENTER) << 4;

    viewCtx.nearClip = VIEWCTX_NEAR_CLIP;
    viewCtx.farClip = VIEWCTX_FAR_CLIP;
    viewCtx.xratio = VIEWCTX_DEFAULT_XRATIO;

    float halfScale = VIEWCTX_HALF_PIXEL;
    viewCtx.xpix = func_001FA6C0(x) * halfScale;
    // The original reloads paramY signed here (a fresh `lh`) rather than
    // reusing the `y` local; matching that keeps the register allocation.
    float yScale = func_001FA6C0(*(const s16*)&occlCamParamBase.paramY) * halfScale;
    viewCtx.ypix = yScale;
    viewCtx.xclip = viewCtx.xpix * VIEWCTX_CLIP_SCALE;
    viewCtx.yclip = yScale * VIEWCTX_CLIP_SCALE;

    viewCtx.fogFarDist = VIEWCTX_DEFAULT_FOG_FAR_DIST;
    viewCtx.fogNearIntensity = VIEWCTX_DEFAULT_FOG_NEAR_INTENSITY;
    viewCtx.fogNearDist = 0.0f;
    viewCtx.fogFarIntensity = 0.0f;
}

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

// drawEnableMask bits 0-6 enable the base set of per-frame draw stages that
// the normal draw path (state 0/8) runs.
#define DRAW_ENABLE_MASK_BASE 0x7F

// Per-frame draw setup for the normal draw states (state 0 and 8): append the
// large framebuffer setup, reset the draw-enable mask to its base set, then
// run the main draw pass. Skipped while a space load is in progress.
extern int spaceLoadInProgress;
extern int drawEnableMask;
// GPREL store alias of drawEnableMask: the mask reset is a GPREL16 store in
// the DrawDebugProfiler call delay slot; the .extern seed makes ps2eeas expand
// the bare pseudo as GPREL16 (a plain declaration would be self-based).
asm(".extern drawEnableMask, 4");
void framebuf_appendLargeSetup(void);
// C linkage: DrawDebugProfiler is supplied by the generated INCLUDE_ASM with
// the unmangled symbol DrawDebugProfiler; the call target is that unmangled
// entry point, which C++ mangling would not resolve.
extern "C" void DrawDebugProfiler(void);

void drawNormalFrame(void) {
    if (spaceLoadInProgress) {
        return;
    }
    framebuf_appendLargeSetup();
    drawEnableMask = DRAW_ENABLE_MASK_BASE;
    DrawDebugProfiler();
}

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
