#include "common.h"
#include "types.h"
#include "camera.h"
#include "actuator.h"
#include "video.h"
#include "sce_gs.h"
#include "hud.h"

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
    int rawX = occlCamParamBase.drawW;
    int rawY = occlCamParamBase.drawH;
    // The params are 16-bit fixed-point; sign-extend and keep a halved copy.
    // EGC 2.95.2 quirk (probe-verified): `x = (s16)raw` lowers to the raw
    // zero-extended value and `(s16)raw >> 1` lowers to the sign-extended
    // value with the >>1 dropped, so the matched bytes store the unhalved
    // params in the halfX/halfY slots. Do not "fix" the >>1 away or add
    // explicit extends; this exact source form is what matches.
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
    // The original reloads drawH signed here (a fresh `lh`) rather than
    // reusing the `y` local; matching that keeps the register allocation.
    float yScale = func_001FA6C0(*(const s16*)&occlCamParamBase.drawH) * halfScale;
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

// Per-level draw-state reset (called from the level and space loaders): zero
// the four per-draw-phase callback-list counters, the effect-quad count, the
// debug/profiler state block, the occlusion-sample VU-chain pulse state,
// OcclMode, and the per-cell occlusion-sample table state.
extern int drawCallbackCount;
extern int drawCallback3Count;
extern int drawCallback4Count;
extern int drawCallback2Count;
extern int effectQuadCount;
// Debug/profiler state in the 0x15F330-0x15F37C block; write-only in the boot
// ELF (no readers found), so the address-based name is retained.
extern int D_0015F330;
extern int D_0015F334;
extern int occlChainActive;
extern int occlChainFrames;
extern int OcclMode;
extern int occlSampleBase;
extern int occlCellTable;
extern int occlCellCount;
// These four are stored GPREL16 here (sw off gp) rather than self-based; the
// .extern seed makes ps2eeas expand the bare pseudo as GPREL16 (a plain
// declaration expands self-based).
extern int screenOverlayEnabled;
// Overlay parameter in the screen-overlay state block; write-only in the boot
// ELF (no readers found), so the address-based name is retained.
extern int D_0015F360;
extern int occlDebugOverlayEnabled;
extern int mobyOcclClusterCount;
asm(".extern screenOverlayEnabled, 4");
asm(".extern D_0015F360, 4");
asm(".extern occlDebugOverlayEnabled, 4");
asm(".extern mobyOcclClusterCount, 4");
// DrawScreenEffect re-reads this pointer GPREL before every field deref; the
// .extern seed makes ps2eeas expand the bare pseudo as GPREL16 (a plain
// declaration would expand self-based).
asm(".extern screenColorEffectNow, 8");

// C linkage: the level loader (func_001E9B10) and space loader (func_00230F60)
// call the unmangled symbol ResetDrawGlobals; a C++ declaration would mangle it
// to ResetDrawGlobals__Fv and fail to link.
extern "C" void ResetDrawGlobals(void) {
    drawCallbackCount = 0;
    drawCallback3Count = 0;
    drawCallback4Count = 0;
    drawCallback2Count = 0;
    effectQuadCount = 0;
    D_0015F330 = 0;
    D_0015F334 = 0;
    occlChainActive = 0;
    occlChainFrames = 0;
    screenOverlayEnabled = 0;
    D_0015F360 = 0;
    occlDebugOverlayEnabled = 0;
    OcclMode = 0;
    occlSampleBase = 0;
    occlCellTable = 0;
    occlCellCount = 0;
    mobyOcclClusterCount = 0;
}

// Head of the VU1 command chain; draw functions append VIF data-reference
// packets through it. Double volatile so EGC re-reads the head before every
// packet store; the plain (non-.data) declaration keeps the load a bare
// self-based pseudo and leaves the streamed block addresses as schedulable
// lui/addiu pairs, matching the original. vuchain.cpp's .data form pairs
// with -mno-split-addresses for that TU instead.
extern volatile u32* volatile vu1ChainHead;
// Same object as vu1ChainHead: the final head update stores GPREL in the
// VU1_addGSregister call delay slot, so it goes through this plain alias.
extern volatile u32* vu1ChainHeadStore;
// 44-word GS state block streamed by the second packet below.
extern u32 vu1GsRegsFont[];
// GS register reset block streamed by the first packet below.
extern u32 resetGsRegsFixed[];
// The original calls it with only the register and value; the third bool
// parameter of the exported symbol is never materialized at any call site.
void VU1_addGSregister(unsigned int reg, unsigned long value)
    asm("VU1_addGSregister__FUiUlb");
// Full-width rectangle overlay VU1 appender (top, bot, left, right, color).
// color is a 64-bit value passed in a single GPR.
void DrawRectOverlay(int top, int bot, int left, int right, unsigned long color)
    asm("DrawRectOverlay_FiiiiUl");

// VIF packet tags for the data-reference records appended below (same
// values vuchain.cpp uses for its chain appenders).
#define VU1_DATA_REF_TAG 0x30000000
#define VU1_DATA_REF_END_TAG 0x50000000
// GS register that receives the packed fog color (R | G<<8 | B<<16); the
// offset is hardware-defined and unnamed in the available references.
#define VU1_FOG_COLOR_GS_REG 0x3D

// Stream the fixed-GS-state and font-GS-state reset blocks into the VU1
// command chain, then load the current fog color into the fog GS register.
void ResetGsRegisters() {
    vu1ChainHead[0] = VU1_DATA_REF_TAG | 0x13;
    vu1ChainHead[1] = (u32)resetGsRegsFixed;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 0x13;
    volatile u32* next = vu1ChainHead + 4;
    vu1ChainHead = next;
    next[0] = VU1_DATA_REF_TAG | 0x0B;
    vu1ChainHead[1] = (u32)vu1GsRegsFont;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 0x0B;
    vu1ChainHeadStore = vu1ChainHead + 4;
    VU1_addGSregister(VU1_FOG_COLOR_GS_REG, (long)viewCtx.fogR | ((long)viewCtx.fogG << 8) | ((long)viewCtx.fogB << 0x10));
}

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

// One effect texture record zeroed when a gif page is set up. Deadlocked's
// EffectTex; the 8-byte tex0 word holds the packed GS tex0 register.
typedef struct {
    u64 tex0;
    u16 ptex;
    u16 ppal;
    u8 uLog;
    u8 vLog;
    s16 format;
} EffectTex;

// Reserves the 16-byte VU1 data-reference slot that DoGifPaging will fill
// (gifPageMarkerA), advances the chain head past it, resets the texture
// cursor to the end of the texture pool, clears the effect texture array,
// and the gif load slot counter. Unless noHud is set, it then invalidates
// the HUD texture and palette GS RAM slots: texture slots at or above the
// start of the texture pool and all palette slots.
extern volatile u32* gifPageMarkerA;
extern int effectTexCnt;
extern EffectTex effectTexs[];
extern int textureCursor;
extern int textureCursorEnd;
// The original reloads this in every HUD texture loop iteration; a plain
// declaration CSEs the load out of the loop.
extern volatile int textureMemoryBase;
// gifLoadCnt (0x15F458) counts the 16-byte load slots that _ssp_load_tex
// allocates from 0x18D020 (slot = count * 16). The clear here is a GPREL16
// store in the branch delay slot; the .extern-seeded alias makes ps2eeas
// expand the bare pseudo as GPREL16 (a plain declaration would be self-based).
extern int gifLoadCntGp;
asm(".extern gifLoadCntGp, 4");

// BLOCKED (2026-09-29): head block, EffectTex memset, both HUD loop BODIES and
// the epilogue all match byte-for-byte; the only residual is the two HUD-loop
// PREAMBLES. EGC 2.95.2 hoists the independent counter zero-init before the
// guard (and will not place an opaque asm into the plain guard's delay slot),
// while the original sits in the delay slot — see
// decomp_state/notes/draw_post_SetupGifPaging__Fi.md and the blocker entry.
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

// The registered callback is a 32-bit function pointer taking the u32
// argument stored next to it; the executors call func(arg) per pair.
typedef void (*DrawCallbackProc)(u32);

// Calls each (func, arg) pair registered on the draw callback list,
// re-reading the list count after every call.
void ExecuteDrawCallbacks(void) {
    int i;
    for (i = 0; i < drawCallbackCount; i++) {
        ((DrawCallbackProc)drawCallbackFuncs[i])(drawCallbackArgs[i]);
    }
}

extern DrawCallbackProc drawCallback3Funcs[];
extern u32 drawCallback3Args[];

// Calls each (func, arg) pair registered on the "pre effects" callback list,
// re-reading the list count after every call.
void ExecuteDrawCallbacks3(void) {
    int i;
    for (i = 0; i < drawCallback3Count; i++) {
        drawCallback3Funcs[i](drawCallback3Args[i]);
    }
}

extern DrawCallbackProc drawCallback4Funcs[];
extern u32 drawCallback4Args[];

// Calls each (func, arg) pair registered on the "vu effects" callback list,
// re-reading the list count after every call.
void ExecuteDrawCallbacks4(void) {
    int i;
    for (i = 0; i < drawCallback4Count; i++) {
        drawCallback4Funcs[i](drawCallback4Args[i]);
    }
}

// Second per-draw-phase callback list (AddDrawCallback is the first). The
// unmangled label is the Splat placeholder for this stripped-ELF symbol.
extern u32 drawCallback2Funcs[];
extern u32 drawCallback2Args[];

void AddDrawCallback2(u32 func, u32 arg) asm("func_001F47B8");
void AddDrawCallback2(u32 func, u32 arg) {
    int idx = drawCallback2Count;
    if (idx < DRAW_CALLBACK_MAX) {
        drawCallback2Funcs[idx] = func;
        drawCallback2Args[idx] = arg;
        drawCallback2Count = idx + 1;
    }
}

// Calls each (func, arg) pair registered on the second per-draw-phase
// callback list, re-reading the list count after every call.
void ExecuteDrawCallbacks2(void) {
    int i;
    for (i = 0; i < drawCallback2Count; i++) {
        ((DrawCallbackProc)drawCallback2Funcs[i])(drawCallback2Args[i]);
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F4880);

// VU1/draw-chain helper prototypes this function drives. VU1_syncChain's mode
// 1 waits for the chain to drain before continuing.
#define VU_SYNC_WAIT 1
void VU1_syncChain(int mode);
void VU1_sendChain(void);
void VU1_swapChain(void);
void PutDrawBufferLarge(void);
void PutDrawBufferSmall(void);
// C linkage: GS status query in the SCE SDK library; the argument is unused.
extern "C" int func_00122298(int arg);
// C linkage: loads the fade GS register (r | g<<8 | b<<16 | intensity<<24) and
// re-streams the fade GS state block; the stripped-ELF symbol is unmangled.
extern "C" void fadeSetColor(int r, int g, int b, int intensity);
// Per-frame draw counter shared with the boot intro. Its load is self-based
// absolute and its store lands GPREL in the VU1_initChain/VU1_sendChain call
// delay slots, so a plain declaration matches the original.
extern int drawFrameCount;
// 80-word GS state block streamed into the VU1 chain on every fade frame.
extern u32 gsStateFade[];
// 80-word GS state block streamed by the fade-color helper (0x1F5210).
extern u32 gsStateFadeColor[];
// GS register that receives the per-frame fade value; the hardware register's
// role is unconfirmed beyond being the fade target.
#define VU1_FADE_GS_REG 1
// Full value on the fade scale; 0x80 in the register's top byte is fully black.
#define VU1_FADE_FULL 0x80

// Symbol override: the stripped-ELF symbol is mangled FadeToBlack__FiUi (int,
// unsigned int) but no boot caller materializes a1, so the source declares a
// single argument and pins the two-argument mangled name.
void FadeToBlack(int frames) asm("FadeToBlack__FiUi");

// Fades the display to black over `frames` frames. Each frame re-sets the draw
// buffers, writes the GS color register and the stepped alpha register,
// re-streams the GS state block, and advances the frame counter.
void FadeToBlack(int frames) {
    int i;
    VU1_syncChain(VU_SYNC_WAIT);
    func_00122298(0);
    drawFrameCount++;
    VU1_initChain();
    i = frames - 1;
    for (; i >= 0; i--) {
        PutDrawBufferLarge();
        framebuf_appendLargeSetup();
        fadeSetColor(0, 0, 0, VU1_FADE_FULL);
        PutDrawBufferSmall();
        VU1_addGSregister(VU1_FADE_GS_REG,
                          (unsigned long)(VU1_FADE_FULL - (i << 7) / (i + 1)) << 24);
        vu1ChainHead[0] = VU1_DATA_REF_TAG | 0x14;
        vu1ChainHead[1] = (u32)gsStateFade;
        vu1ChainHead[2] = 0;
        vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 0x14;
        vu1ChainHeadStore = vu1ChainHead + 4;
        VU1_syncChain(VU_SYNC_WAIT);
        func_00122298(0);
        drawFrameCount++;
        VU1_sendChain();
        VU1_swapChain();
    }
    VU1_syncChain(VU_SYNC_WAIT);
    func_00122298(0);
    drawFrameCount++;
    VU1_initChain();
    PutDrawBufferLarge();
    framebuf_appendLargeSetup();
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F4BE0);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F4D98);

// Screen VBlank color-effect renderer (background fill + two scanline bands).
// Reads screenColorEffectNow (GPREL) before every field deref; the .extern
// seed above keeps those loads GPREL16.
//
// count is pinned to s1 ($17) and pos left a plain local: with all five
// callee-saved registers live EGC re-reads the pointer after each call and
// recomputes count-1 per band instead of hoisting it, and pinning pos would
// make it emit a slt/beqz pre-check instead of the original's single blez.
// GS register that receives the screen-effect alpha; the register's role is
// unconfirmed beyond being the effect's alpha target.
#define VU1_SCREEN_ALPHA_GS_REG 0x42
// Mask applied to each alpha value before it is sent to the GS register.
#define SCREEN_EFFECT_ALPHA_MASK 0xFF000000FFUL
// A band's color is enabled by its top byte.
#define SCREEN_EFFECT_COLOR_ENABLE 0xFF000000

void DrawScreenEffect() {
    s32 pos = 0;
    register s32 count asm("$17") = (s16)occlCamParamBase.drawH;
    if (screenColorEffectNow->bkgAlpha != 0)
        VU1_addGSregister(VU1_SCREEN_ALPHA_GS_REG,
                          screenColorEffectNow->bkgAlpha & SCREEN_EFFECT_ALPHA_MASK);
    if ((screenColorEffectNow->bkgColor & SCREEN_EFFECT_COLOR_ENABLE) != 0)
        DrawRectOverlay(0, count, 0, (s16)occlCamParamBase.drawW,
                        (unsigned long)screenColorEffectNow->bkgColor);
    while (pos < count) {
        if (screenColorEffectNow->aAlpha != 0)
            VU1_addGSregister(VU1_SCREEN_ALPHA_GS_REG,
                              screenColorEffectNow->aAlpha & SCREEN_EFFECT_ALPHA_MASK);
        if ((screenColorEffectNow->aColor & SCREEN_EFFECT_COLOR_ENABLE) != 0) {
            s32 end = pos + screenColorEffectNow->aLines;
            if (end >= count - 1)
                end = count - 1;
            DrawRectOverlay(pos, end, 0, (s16)occlCamParamBase.drawW,
                            (unsigned long)screenColorEffectNow->aColor);
        }
        pos += screenColorEffectNow->aLines;
        if (screenColorEffectNow->bAlpha != 0)
            VU1_addGSregister(VU1_SCREEN_ALPHA_GS_REG,
                              screenColorEffectNow->bAlpha & SCREEN_EFFECT_ALPHA_MASK);
        if ((screenColorEffectNow->bColor & SCREEN_EFFECT_COLOR_ENABLE) != 0) {
            s32 end = pos + screenColorEffectNow->bLines;
            if (end >= count - 1)
                end = count - 1;
            DrawRectOverlay(pos, end, 0, (s16)occlCamParamBase.drawW,
                            (unsigned long)screenColorEffectNow->bColor);
        }
        pos += screenColorEffectNow->bLines;
    }
}

// The frame buffer base address (0x15EE88); the overlay GS writes below
// shift it arithmetically, so it is read as a signed int here (a u32 shift
// would lower to srl, not the original's sra).
extern u32 frameBufferBase;

// GS register that receives the frame-buffer-derived overlay base value; the
// offset is hardware-defined and unnamed in the available references.
#define OCCL_DEBUG_GS_REG 0x4E
// Right-shift applied to the frame buffer base before it is ORed into the
// overlay GS value (byte address to the base's hardware addressing unit).
#define OCCL_DEBUG_FB_SHIFT 13
// Bits ORed into the frame-buffer-derived overlay base for the two GS writes
// around the overlay rectangle: the common bit is used by both, the leading
// bit only by the write that precedes the rectangle.
#define OCCL_DEBUG_BASE_LEAD 0x100000000
#define OCCL_DEBUG_BASE_COMMON 0x1000000
// Fixed alpha sent to the alpha GS register after the overlay rectangle,
// closing the pair opened by the masked bkgAlpha write at the top.
#define OCCL_DEBUG_END_ALPHA 0x8000000044UL

// Occlusion-debug overlay renderer, the occlusion twin of DrawScreenEffect.
// Takes the debug overlay's ScreenVBEffect config (the block based at
// occlDebugOverlayEnabled, 0x15F370) instead of the global
// screenColorEffectNow, and draws only its background (no A/B bands). An
// enabled bkgColor wraps the full-screen DrawRectOverlay in two
// OCCL_DEBUG_GS_REG writes derived from the frame buffer base. The caller
// (DrawDebugProfiler) references the original placeholder symbol, so the
// declaration keeps that name via the asm override.
void DrawOcclDebugOverlay(ScreenVBEffect* effect) asm("func_001F5138");
void DrawOcclDebugOverlay(ScreenVBEffect* effect) {
    if (effect->bkgAlpha != 0)
        VU1_addGSregister(VU1_SCREEN_ALPHA_GS_REG,
                          effect->bkgAlpha & SCREEN_EFFECT_ALPHA_MASK);
    if ((effect->bkgColor & SCREEN_EFFECT_COLOR_ENABLE) != 0) {
        // OCCL_DEBUG_BASE_COMMON must stay a bare constant: pinning it to a
        // register makes EGC emit the rectangle-following OR in-place
        // (or a1,a1,s0) instead of the original's or a1,s0,a1.
        VU1_addGSregister(OCCL_DEBUG_GS_REG,
                          (int)frameBufferBase >> OCCL_DEBUG_FB_SHIFT
                              | OCCL_DEBUG_BASE_COMMON | OCCL_DEBUG_BASE_LEAD);
        DrawRectOverlay(0, (s16)occlCamParamBase.drawH, 0, (s16)occlCamParamBase.drawW,
                        (unsigned long)effect->bkgColor);
        VU1_addGSregister(OCCL_DEBUG_GS_REG,
                          (int)frameBufferBase >> OCCL_DEBUG_FB_SHIFT
                              | OCCL_DEBUG_BASE_COMMON);
    }
    if (effect->bkgAlpha != 0)
        VU1_addGSregister(VU1_SCREEN_ALPHA_GS_REG, OCCL_DEBUG_END_ALPHA);
}

void fadeSetColor(int r, int g, int b, int intensity) {
    VU1_addGSregister(VU1_FADE_GS_REG, (unsigned long)r | ((unsigned long)g << 8)
                      | ((unsigned long)b << 0x10) | ((unsigned long)intensity << 0x18));
    vu1ChainHead[0] = VU1_DATA_REF_TAG | 0x14;
    vu1ChainHead[1] = (u32)gsStateFadeColor;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 0x14;
    vu1ChainHead = vu1ChainHead + 4;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", DrawRectOverlay_FiiiiUl);

// Unreachable dead tail the original compiler emitted after
// DrawRectOverlay_FiiiiUl (blocked: EGC RA wall, see
// decomp_state/notes/draw_post_DrawRectOverlay_FiiiiUl.md): a GPREL store of
// the new vu1ChainHead into D_00166C00 (the gp-window base word), the
// compiler's second copy of the final head update landing after the jr, plus
// the alignment nop before DrawTexturedQuad. Nothing reaches it (no Ghidra
// function; tools/deadness_scan.py: 0 references), so the bytes are
// preserved with raw asm per the dead-tail policy; the parent keeps its
// INCLUDE_ASM.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001F5448, 0x4\n"
    "glabel func_001F5448\n"
    "    .word 0xaf820000\n"
    "endlabel func_001F5448\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", DrawTexturedQuad);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F55D8);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F5808);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F5AB0);

// Unreachable dead tail the original compiler emitted after
// func_001F5AB0 (DrawOcclEffectSprite, blocked: EGC RA/scheduler wall, see
// decomp_state/notes/draw_post_func_001F5AB0.md): a GPREL store of the new
// vu1ChainHead into D_00166C00 (the gp-window base word), the compiler's
// second copy of the final head update landing after the jr, plus the
// alignment nop before DrawUIFrame. Nothing reaches it (no Ghidra function;
// tools/deadness_scan.py: 0 references), so the bytes are preserved with raw
// asm per the dead-tail policy; the parent keeps its INCLUDE_ASM.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001F5F10, 0x4\n"
    "glabel func_001F5F10\n"
    "    .word 0xaf820000\n"
    "endlabel func_001F5F10\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

// C linkage: the stripped-ELF symbol is unmangled DrawUIFrame (the sibling
// DrawRectOverlay of the same subsystem is C++ mangled); it belongs to the
// unmangled UI/font API family (fadeSetColor, FontSetWindow, FontPrintWindow).
// Draws a beveled UI frame: the main (top, bot, left, right) rectangle plus
// three stepped bevel bands just outside each vertical edge, each inset 1, 2
// and 4 pixels, in a near-black color whose top byte is the alpha.
// The frame's near-black fill (R | G<<8 | B<<16); only the alpha varies.
#define DRAW_UI_FRAME_SHADOW_RGB 0x40404
extern "C" void DrawUIFrame(int top, int bot, int left, int right, int alpha) {
    unsigned long color = (alpha << 24) | DRAW_UI_FRAME_SHADOW_RGB;
    DrawRectOverlay(top, bot, left, right, color);
    DrawRectOverlay(top + 1, bot - 1, left - 2, left, color);
    DrawRectOverlay(top + 2, bot - 2, left - 3, left - 2, color);
    DrawRectOverlay(top + 4, bot - 4, left - 4, left - 3, color);
    DrawRectOverlay(top + 1, bot - 1, right, right + 2, color);
    DrawRectOverlay(top + 2, bot - 2, right + 2, right + 3, color);
    DrawRectOverlay(top + 4, bot - 4, right + 3, right + 4, color);
}

#define DRAW_BEVEL_FRAME_ALPHA_MASK 0xFF000000
#define DRAW_BEVEL_FRAME_FILL_RGB 4
// Draws a beveled rectangular frame: the main (top, bot, left, right) rect in a
// near-black fill that keeps the input color's top byte (alpha) and forces the
// RGB bytes to a dark value, plus eight stepped bevel bands around all four
// edges drawn in the full input color.
// color must stay a signed int: EGC passes a signed int to DrawRectOverlay's
// 64-bit color arg as a plain register move, but would zero-extend an unsigned
// int (dsll32/dsrl32) which the original does not emit.
// C linkage: unmangled sibling of the confirmed-C-linkage DrawUIFrame, in the
// same UI-draw API family (fadeSetColor, FontPrintWindow, DrawRectOverlay).
extern "C" void DrawBevelFrame(int top, int bot, int left, int right, int color) {
    int fill = (color & DRAW_BEVEL_FRAME_ALPHA_MASK) | DRAW_BEVEL_FRAME_FILL_RGB;
    DrawRectOverlay(top, bot, left, right, fill);
    DrawRectOverlay(top - 1, top + 1, left + 3, right + 5, color);
    DrawRectOverlay(top - 3, top - 5, left - 1, right - 3, color);
    DrawRectOverlay(top - 5, bot - 3, left - 1, left + 1, color);
    DrawRectOverlay(top + 3, bot + 1, left - 3, left - 5, color);
    DrawRectOverlay(bot - 1, bot + 1, left - 3, right - 3, color);
    DrawRectOverlay(bot + 3, bot + 5, left + 3, right + 1, color);
    DrawRectOverlay(top + 3, bot + 3, right - 1, right + 1, color);
    DrawRectOverlay(top + 1, bot - 3, right + 3, right + 5, color);
}

extern int drawOcclusionEnabled;

void enableOcclusion(void) {
    drawOcclusionEnabled = 1;
}

void disableOcclusion(void) {
    drawOcclusionEnabled = 0;
}

// Font glyph table layout: each entry is 4 bytes and the signed advance width
// is the final byte.
#define FONT_GLYPH_STRIDE 4
#define FONT_GLYPH_WIDTH_OFFSET 3

// C linkage: sums the signed glyph width byte over the nonzero text bytes,
// stopping at the first zero byte or when the counter reaches length. The
// original call target is an unmangled entry.
extern "C" int fontMeasureString(u8* text, int length, char* glyphs) {
    int total = 0;
    int i = 0;

    if (length != 0 && text[0] != 0) {
        u8* p = text;
        u8 c = *p;

        do {
            ++i;
            ++p;
            // Tied barrier: blocks EGC edge-splitting the width index (c*stride)
            // ahead of the pointer advance, preserving the original loop head.
            asm volatile("" : "+r"(c) : "r"(p));

            char w = glyphs[c * FONT_GLYPH_STRIDE + FONT_GLYPH_WIDTH_OFFSET];
            if (w != 0)
                total += w;

            if (i == length)
                break;

            c = *p;
        } while (c != 0);
    }

    return total;
}

extern char fontSmallGlyphs[];

int drawTextSmall(char* text, int length) {
    return fontMeasureString((u8*)text, length, fontSmallGlyphs);
}

extern char fontMediumGlyphs[];

int drawTextMedium(char* text, int length) {
    return fontMeasureString((u8*)text, length, fontMediumGlyphs);
}

extern char fontLargeGlyphs[];

int drawTextLarge(char* text, int length) {
    return fontMeasureString((u8*)text, length, fontLargeGlyphs);
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", FontPrint);

// The real GetEffectTex__Fii entry point takes two int parameters, but the
// callee never reads the second and this call site leaves it stale (only a0
// is set before the jal). This EGC rejects a one-argument call to a
// two-parameter prototype, so the one-parameter declaration is pinned to
// the original label.
int GetEffectTex(int texId) asm("GetEffectTex__Fii");

// The per-level effectTexs[] slots bound by the font family: each glyph
// table always pairs with one shadow/effect texture slot (the FontPrint*
// wrappers below bind the slot matching their glyph set).
#define FONT_EFFECT_TEX_SMALL 1
#define FONT_EFFECT_TEX_MEDIUM 2
#define FONT_EFFECT_TEX_LARGE 3

// C linkage: the stripped-ELF FontPrint family symbols are unmangled
// (FontPrint, FontPrintCenter, FontPrintWindow).
extern "C" void FontPrint(int x, int y, int color, u8* text, int length,
                          int tex, char* glyphs);

// C linkage: the stripped-ELF symbol is the unmangled FontPrintLarge, a
// member of the same font API family. Prints text with the fontSmallGlyphs
// set and the FONT_EFFECT_TEX_SMALL slot bound.
extern "C" void FontPrintLarge(int x, int y, int color, u8* text, int length) {
    FontPrint(x, y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_SMALL), fontSmallGlyphs);
}

// C linkage: the stripped-ELF symbol is the unmangled FontPrintSmall, a
// member of the same font API family. Prints text with the fontMediumGlyphs
// set and the FONT_EFFECT_TEX_MEDIUM slot bound (the glyph-table naming is
// inverted vs the wrapper name: FontPrintLarge uses fontSmallGlyphs).
extern "C" void FontPrintSmall(int x, int y, int color, u8* text, int length) {
    FontPrint(x, y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_MEDIUM), fontMediumGlyphs);
}

// Unreachable dead tail after FontPrintSmall (0x1F6630): one `addiu
// sp,sp,0x60` matching the parent's 0x60 frame. Nothing reaches it (0 jal/j/
// branch/data references; no Ghidra function), so it is not a function; the
// original compiler emitted this byte after the parent's RTL, so it is
// preserved here as an exact word. The trailing nop pads to the 8-aligned
// func_001F6638 entry at 0x1F6638.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001F6630, 0x4\n"
    "glabel func_001F6630\n"
    "    .word 0x27BD0060\n"
    "endlabel func_001F6630\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F6638);

// Unreachable dead tail after func_001F6638 (0x1F6928): three `addiu
// sp,sp,0x60; nop` units the original compiler emitted after the parent's
// RTL (the 0x60 matches the next function's frame, not the parent's 0xD0).
// Nothing reaches them (0 jal/j/branch/data references at 0x1F6928/30/38;
// no Ghidra function), so the bytes are preserved here as raw asm per the
// dead-tail policy; the parent keeps its INCLUDE_ASM.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001F6928, 0x12\n"
    "glabel func_001F6928\n"
    "    .word 0x27BD0060\n"
    "    .word 0x00000000\n"
    "    .word 0x27BD0060\n"
    "    .word 0x00000000\n"
    "    .word 0x27BD0060\n"
    "    .word 0x00000000\n"
    "endlabel func_001F6928\n"
    "    .set reorder\n"
    "    .set at\n"
);

// C linkage: right-aligned member of the unmangled FontPrint API family
// (like FontPrintCenter, named after the FontPrintRight/Small/Large trio of
// the direct-descendant Deadlocked build). Prints text so its right edge
// lands at x: the full measured width is subtracted from x (the Center
// siblings subtract half). Uses the fontSmallGlyphs set with the
// FONT_EFFECT_TEX_SMALL slot, the same set and slot FontPrintLarge binds.
extern "C" void FontPrintRight(int x, int y, int color, u8* text, int length) {
    FontPrint(x - drawTextSmall((char*)text, length), y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_SMALL), fontSmallGlyphs);
}

// C linkage: right-aligned member of the unmangled FontPrint API family, the
// medium-glyph variant of FontPrintRight (the "Small" suffix follows the
// family's glyph-set inversion, like FontPrintCenterSmall: it prints with the
// fontMediumGlyphs set and the FONT_EFFECT_TEX_MEDIUM slot). Prints text so
// its right edge lands at x: the full measured width is subtracted from x.
extern "C" void FontPrintRightSmall(int x, int y, int color, u8* text,
                                    int length) {
    FontPrint(x - drawTextMedium((char*)text, length), y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_MEDIUM), fontMediumGlyphs);
}

// C linkage: right-aligned member of the unmangled FontPrint API family, the
// large-glyph variant of FontPrintRight (the "Large" suffix follows the
// family's glyph-set inversion, like FontPrintCenterLarge: it prints with the
// fontLargeGlyphs set and the FONT_EFFECT_TEX_LARGE slot). Prints text so its
// right edge lands at x: the full measured width is subtracted from x.
extern "C" void FontPrintRightLarge(int x, int y, int color, u8* text,
                                    int length) {
    FontPrint(x - drawTextLarge((char*)text, length), y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_LARGE), fontLargeGlyphs);
}

// C linkage: center-aligned member of the unmangled FontPrint API family.
// Prints the text centered on x (x minus half the measured width) with the
// fontSmallGlyphs set and the FONT_EFFECT_TEX_SMALL slot, and returns the
// centered x (the original leaves it in v0 after the call).
extern "C" int FontPrintCenter(int x, int y, int color, u8* text, int length) {
    int cx = x - (drawTextSmall((char*)text, length) >> 1);
    FontPrint(cx, y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_SMALL), fontSmallGlyphs);
    return cx;
}

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

extern u32 bitSwapLut[256] __attribute__((section(".data")));

void buildBitSwapLut(void) {
    for (int i = 0; i < 256; i++) {
        int index = i & 0xE7;
        if (i & 0x8) index |= 0x10;
        if (i & 0x10) index |= 0x8;
        bitSwapLut[index] = (u32)((i >> 1) << 24);
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F7A88);
