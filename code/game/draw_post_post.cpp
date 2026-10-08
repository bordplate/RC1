#include "common.h"
#include "types.h"
#include "camera.h"
#include "actuator.h"
#include "video.h"
#include "sce_gs.h"
#include "hud.h"
#include "font.h"

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
extern u32 vu1GsRegsFont[44];
// 76-word GS register reset block streamed by the first packet below.
extern u32 resetGsRegsFixed[76];
// The original calls it with only the register and value; the third bool
// parameter of the exported symbol is never materialized at any call site.
void VU1_addGSregister(unsigned int reg, unsigned long value)
    asm("VU1_addGSregister__FUiUlb");
// Full-width rectangle overlay VU1 appender (top, bot, left, right, color).
// color is a 64-bit value passed in a single GPR.
void DrawRectOverlay(int top, int bot, int left, int right, unsigned long color)
    asm("DrawRectOverlay_FiiiiUl");

// VIF packet tags for the data-reference records appended below (same
// values vuchain.cpp uses for its chain appenders). The low byte (qcnt) is
// the streamed block's size in 16-byte units, derived from the block below.
#define VU1_DATA_REF_TAG 0x30000000
#define VU1_DATA_REF_END_TAG 0x50000000
// GS register that receives the packed fog color (R | G<<8 | B<<16); the
// offset is hardware-defined and unnamed in the available references.
#define VU1_FOG_COLOR_GS_REG 0x3D

// Stream the fixed-GS-state and font-GS-state reset blocks into the VU1
// command chain, then load the current fog color into the fog GS register.
void ResetGsRegisters() {
    vu1ChainHead[0] = VU1_DATA_REF_TAG | (sizeof(resetGsRegsFixed) / 16);
    vu1ChainHead[1] = (u32)resetGsRegsFixed;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | (sizeof(resetGsRegsFixed) / 16);
    volatile u32* next = vu1ChainHead + 4;
    vu1ChainHead = next;
    next[0] = VU1_DATA_REF_TAG | (sizeof(vu1GsRegsFont) / 16);
    vu1ChainHead[1] = (u32)vu1GsRegsFont;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | (sizeof(vu1GsRegsFont) / 16);
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
void appendClearBlackDataRef(void);
// C linkage: DrawDebugProfiler is supplied by the generated INCLUDE_ASM with
// the unmangled symbol DrawDebugProfiler; the call target is that unmangled
// entry point, which C++ mangling would not resolve.
extern "C" void DrawDebugProfiler(void);

void drawNormalFrame(void) {
    if (spaceLoadInProgress) {
        return;
    }
    appendClearBlackDataRef();
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
extern u32 gsStateFade[80];
// 80-word GS state block streamed by the fade-color helper (0x1F5210).
extern u32 gsStateFadeColor[80];
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
        appendClearBlackDataRef();
        fadeSetColor(0, 0, 0, VU1_FADE_FULL);
        PutDrawBufferSmall();
        VU1_addGSregister(VU1_FADE_GS_REG,
                          (unsigned long)(VU1_FADE_FULL - (i << 7) / (i + 1)) << 24);
        vu1ChainHead[0] = VU1_DATA_REF_TAG | (sizeof(gsStateFade) / 16);
        vu1ChainHead[1] = (u32)gsStateFade;
        vu1ChainHead[2] = 0;
        vu1ChainHead[3] = VU1_DATA_REF_END_TAG | (sizeof(gsStateFade) / 16);
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
    appendClearBlackDataRef();
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
    vu1ChainHead[0] = VU1_DATA_REF_TAG | (sizeof(gsStateFadeColor) / 16);
    vu1ChainHead[1] = (u32)gsStateFadeColor;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | (sizeof(gsStateFadeColor) / 16);
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
// two-parameter prototype, so the one-parameter declaration is pinned to the
// original label. NOTE: the entry actually returns a 64-bit tex0 word (a 64-bit
// ld in its epilogue); this 32-bit return is what the font call sites (which
// pass the result to an int tex parameter) match with. help.cpp declares it
// 64-bit because Help_DrawPrompt uses the full word; see the note there.
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

// C linkage: center-aligned member of the unmangled FontPrint API family.
// Prints the text centered on x (x minus half the measured width) with the
// fontMediumGlyphs set and the FONT_EFFECT_TEX_MEDIUM slot, and returns the
// centered x (the original leaves it in v0 after the call).
extern "C" int FontPrintCenterSmall(int x, int y, int color, u8* text,
                                    int length) {
    int cx = x - (drawTextMedium((char*)text, length) >> 1);
    FontPrint(cx, y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_MEDIUM), fontMediumGlyphs);
    return cx;
}

// C linkage: center-aligned member of the unmangled FontPrint API family.
// Prints the text centered on x (x minus half the measured width) with the
// fontLargeGlyphs set and the FONT_EFFECT_TEX_LARGE slot, and returns the
// centered x (the original leaves it in v0 after the call).
extern "C" int FontPrintCenterLarge(int x, int y, int color, u8* text,
                                    int length) {
    int cx = x - (drawTextLarge((char*)text, length) >> 1);
    FontPrint(cx, y, color, text, length,
              GetEffectTex(FONT_EFFECT_TEX_LARGE), fontLargeGlyphs);
    return cx;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", func_001F6CB8);

// One glyph-table entry (fontSmall/Medium/LargeGlyphs): texture coords u/v
// as u8, then signed vertical drop and horizontal advance (verified against
// the field accesses in the FontPrint family; see
// decomp_state/notes/draw_post_func_001F6CB8.md).
typedef struct fontLetter {
    u8 u;
    u8 v;
    s8 drop;
    s8 advance;
} fontLetter;

// C linkage target is still the Splat placeholder: func_001F6CB8 is a
// blocked target (see decomp_state/blocked.json), so the descriptive name
// binds to the placeholder label with an asm override. The 9th argument
// (glyphs) is passed in the 0(sp) stack slot by the EGC register window.
extern "C" int FontPrintWindowGeneric(int x, int y, int width, int height,
                                      long color, u8* text, int length,
                                      int effect, fontLetter* glyphs)
    asm("func_001F6CB8");

// C linkage: the stripped-ELF symbol is the unmangled FontPrintWindowSmall,
// the windowed-font member of the FontPrint API family. Prints text in a
// width-bounded window starting at (x, y) with the fontMediumGlyphs set and
// the FONT_EFFECT_TEX_MEDIUM slot bound. FontPrintWindowGeneric's return
// (lines used, as final line y minus y plus 0x10) passes through in v0
// untouched; callers use it to decide line advance.
extern "C" void FontPrintWindowSmall(int x, int y, int width, int height,
                                     long color, u8* text, int length) {
    FontPrintWindowGeneric(x, y, width, height, color, text, length,
                           GetEffectTex(FONT_EFFECT_TEX_MEDIUM),
                           (fontLetter*)fontMediumGlyphs);
}

// Unreachable dead tail after FontPrintWindowSmall (0x1F7070): an `addiu
// sp,sp,0x100` plus three `addiu sp,sp,0x70; nop` units the original
// compiler emitted after the parent's RTL. Nothing reaches it (0 jal/j/
// branch/data references; no Ghidra function), so it is not a function; the
// bytes are preserved as exact words. The trailing nop pads to the
// FontPrintWindow entry at 0x1F7090.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001F7070, 0x1C\n"
    "glabel func_001F7070\n"
    "    .word 0x27BD0100\n"
    "    .word 0x00000000\n"
    "    .word 0x27BD0070\n"
    "    .word 0x00000000\n"
    "    .word 0x27BD0070\n"
    "    .word 0x00000000\n"
    "    .word 0x27BD0070\n"
    "endlabel func_001F7070\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

// FontWindow is defined in font.h (shared with help.cpp).

INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", FontPrintWindow);

// C linkage: FontPrintWindow is supplied by the generated INCLUDE_ASM
// fallback (blocked: EGC register-allocation wall, see
// decomp_state/blocked.json); this prototype states the EGC register-window
// ABI the generated assembly implements: (f, rgba, text, length) in a0-a3,
// (tex, glyphs) in t0/t1.
extern "C" void FontPrintWindow(FontWindow* f, long rgba, u8* text, int length,
                                long tex, fontLetter* glyphs);

// C linkage: the stripped-ELF symbol is the unmangled FontPrintWindowLarge,
// a member of the unmangled FontPrint API family (the Deadlocked build has
// the same two-arg-count FontPrintWindowSmall/Large pair). Prints text in a
// width-bounded window described by the FontWindow with the fontSmallGlyphs
// set and the FONT_EFFECT_TEX_SMALL slot bound, the same set and slot
// FontPrintLarge binds (the family's glyph-table naming is inverted vs the
// wrapper name).
extern "C" void FontPrintWindowLarge(FontWindow* f, long color, u8* text,
                                     int length) {
    FontPrintWindow(f, color, text, length,
                    GetEffectTex(FONT_EFFECT_TEX_SMALL), (fontLetter*)fontSmallGlyphs);
}

// C linkage: the stripped-ELF symbol is the unmangled FontPrintWindowMedium,
// the medium-glyph member of the unmangled FontPrint API family. Prints text
// in the width-bounded window described by the FontWindow with the
// fontMediumGlyphs set and the FONT_EFFECT_TEX_MEDIUM slot bound (the
// FontWindow-based sibling FontPrintWindowLarge binds fontSmallGlyphs; the
// family's glyph-table naming is inverted vs the wrapper name, as there).
extern "C" void FontPrintWindowMedium(FontWindow* f, long color, u8* text,
                                      int length) {
    FontPrintWindow(f, color, text, length,
                    GetEffectTex(FONT_EFFECT_TEX_MEDIUM), (fontLetter*)fontMediumGlyphs);
}

// Unreachable dead tail after FontPrintWindowMedium (0x1F75F0): one
// `addiu sp,sp,0x50` matching the parent's 0x50 frame. Nothing reaches it
// (tools/deadness_scan.py 0x1F7660 -> 0 references; no Ghidra function), so
// it is not a function; the original compiler emitted this byte after the
// parent's RTL, so it is preserved here as an exact word. The trailing nop
// pads to the 8-aligned FontSetWindow entry at 0x1F7668.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001F7660, 0x4\n"
    "glabel func_001F7660\n"
    "    .word 0x27BD0050\n"
    "endlabel func_001F7660\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

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

// C linkage: part of the unmangled font API family (FontSetWindow,
// FontPrintWindow, ...).
extern "C" void draw_loadViewMatrixW(void* a0, float w);
void VU1_addDataRef(void* dataRef, s32 qcnt);
void VU1_gsRegsFont(void);
extern int fontState;
extern u16 fontVUProgramTag __attribute__((section(".data")));
extern u32 fontVUProgram[];
// GPREL access alias of fontDepthBias: the record matrix rows add it as a
// GP-relative float load, while the plain symbol stays self-based absolute.
extern float fontDepthBiasGp;
asm(".extern fontDepthBiasGp, 4");

// VU1 font state record words (0xF0 bytes queued for the font render
// program); meanings beyond the view-context snapshot are not yet resolved.
#define FONT_STATE_TAG     0x10000000u // word 0 base; OR'd with the queue length (words >> 4 - 1) at the end
#define FONT_STATE_TAG2    0x11000000u
#define FONT_STATE_TAG3    0x01000404u
#define FONT_STATE_COLOR   0x006C0C43A4u
#define FONT_STATE_W0xA0   0x00008000u
#define FONT_STATE_W0xA4   0x00303EC000u
#define FONT_STATE_W0xA8   0x00000412u
#define FONT_STATE_W0xE0   0x003000000u
#define FONT_STATE_W0xE4   0x0020001D2u
#define FONT_STATE_W0xE8   0x0015000000u

// Queue the per-frame font VU1 state: build the view rows (scaled camera
// position plus the VU-side view matrix), stream the font VU program on
// first use, and append the 0xF0-byte camera/fog record the font render
// program consumes. Runs once per font frame while windows are enabled.
extern "C" void FontQueueVUState(void) {
    float viewRows[16];
    int fontInit = 7;
    draw_loadViewMatrixW(viewRows, 1024.0f);
    FastVecScale(viewRows + 12, &currentCamera.pos, -1024.0f);
    viewRows[15] = 1.0f;
    if (fontState != fontInit) {
        VU1_addDataRef(fontVUProgram, fontVUProgramTag);
        fontState = fontInit;
    }
    register u32 tag2 asm("$8") = FONT_STATE_TAG2;
    u32 color = FONT_STATE_COLOR;
    vu1ChainHead[0] = FONT_STATE_TAG;
    vu1ChainHead[1] = 0;
    vu1ChainHead[2] = tag2;
    vu1ChainHead[3] = FONT_STATE_TAG3;
    u32* rec = (u32*)vu1ChainHead;
    u32* p = rec + 8;
    rec[4] = 0;
    rec[5] = 0;
    rec[6] = 0;
    rec[7] = color;
    draw_transformMatrix(p, currentCamera.matrix, viewRows);
    ((float*)p)[14] += fontDepthBiasGp;
    p = rec + 0x18;
    draw_transformMatrix(p, currentCamera.mtx3, viewRows);
    ((float*)p)[14] += fontDepthBiasGp;
    rec[0x28] = FONT_STATE_W0xA0;
    p = rec + 0x2C;
    rec[0x29] = FONT_STATE_W0xA4;
    rec[0x2A] = FONT_STATE_W0xA8;
    register ViewCtx* ctx asm("$4") = &viewCtx;
    ((float*)rec)[0x2B] = ctx->perspScale;
    // The two 128-bit quad copies pin their source pointers (a1/v1) and tie the
    // destination so EGC materializes `sq v0,0(s1)` instead of folding to
    // `sq imm(s0)`. The hvdf temp is pinned to the v0:v1 pair to match the
    // original's overlapping `lq v0,0(v1)` (src in v1); the tied barrier after
    // the clip store keeps the hvdf-src setup from hoisting past it.
    register CameraQuad* clip asm("$5") = &ctx->fontClipScale;
    asm volatile("" : "+r"(clip));
    asm volatile("" : "+r"(p));
    *(CameraQuad*)p = *clip;
    asm volatile("" : : : "memory");
    p = rec + 0x30;
    register CameraQuad* hv asm("$3") = (CameraQuad*)ctx->hvdf;
    asm volatile("" : "+r"(hv));
    register CameraQuad hvq asm("$2");
    hvq = *hv;
    asm volatile("" : "+r"(p));
    *(CameraQuad*)p = hvq;
    asm volatile("" : : : "$2", "$3", "$5", "memory");
    ((float*)rec)[0x34] = ctx->fogFarIntensity;
    register u32 e0 asm("$3") = FONT_STATE_W0xE0;
    register u32 e4 asm("$2") = FONT_STATE_W0xE4;
    register u32 e8 asm("$5") = FONT_STATE_W0xE8;
    p = rec + 0x3C;
    ((float*)rec)[0x35] = ctx->fogNearIntensity;
    rec[0x39] = e4;
    rec[0x3A] = e8;
    rec[0x38] = e0;
    rec[0x37] = 0;
    rec[0x3B] = 0;
    rec[0x36] = 0;
    // The memory barrier keeps the head load from interleaving with the record
    // stores; the diff/word pins and tied barriers reproduce the original's
    // `subu; lw; sra; addiu; or; sw` queue-length RMW (diff in v0, word in v1).
    asm volatile("" : : : "memory");
    register u32* head asm("$4") = (u32*)vu1ChainHead;
    register int diff asm("$2") = (int)p - (int)head;
    asm volatile("" : "+r"(diff));
    register u32 word asm("$3") = head[0];
    asm volatile("" : "+r"(diff), "+r"(word));
    diff >>= 4;
    diff -= 1;
    word |= diff;
    head[0] = word;
    vu1ChainHeadStore = p;
    VU1_gsRegsFont();
}

// func_001F33B8 and func_001FB440 are still INCLUDE_ASM placeholders; the asm
// labels bind these calls to the unmangled symbols the generated assembly
// defines, which C++ mangling would not produce.
void func_001F33B8(int width, int height, float xratio, float fogNearDist,
                   float fogFarDist, float fogNearIntensity, float fogFarIntensity)
    asm("func_001F33B8");
void func_001FB440(int log2Width, int log2Height, int gsBase)
    asm("func_001FB440");

// Sets up the occlusion effect draw buffer. inPlace != 0 uses the main effect
// buffer (occlCamParamBase.effectBufBaseGs); otherwise it reserves a scratch
// buffer below the texture pool. The caller (func_002196B8) still references
// func_001F7888, so keep that symbol via the asm override; cfront cannot take
// the label on the definition, so it lives on this declaration.
void setupEffectDrawBuffer(int log2Width, int log2Height, int inPlace, float xratio)
    asm("func_001F7888");

void setupEffectDrawBuffer(int log2Width, int log2Height, int inPlace, float xratio) {
    float x = xratio;
    register int base asm("$3");
    if (inPlace != 0) {
        base = occlCamParamBase.effectBufBaseGs;
    } else {
        base = log2Width + log2Height;
        // Pins + input-only barriers reproduce EGC's clamp (slti/movz) register
        // map and the textureBase / `4 << t` scheduling. See
        // decomp_state/notes/draw_post_post_setupEffectDrawBuffer.md.
        register int limit asm("$6") = 0x10;
        register int belowLimit asm("$4") = base < 0x11;
        asm volatile("" : : "r"(belowLimit));
        register int four asm("$2") = 4;
        asm volatile("" : : "r"(four));
        if (!belowLimit)
            base = limit;
        register int textureBase asm("$5") = textureMemoryBase;
        four <<= base;
        register int difference asm("$6") = textureBase - four;
        base = difference >> 13;
    }
    // The gsBase anchor keeps the `sll a2, v1, 13` in the body ahead of the arg
    // moves instead of the jal delay slot.
    register int gsBase asm("$6") = base << 13;
    asm volatile("" : : "r"(gsBase));
    func_001FB440(log2Width, log2Height, gsBase);
    func_001F33B8(1 << log2Width, 1 << log2Height, x, 0.0f, 524288.0f, 255.0f, 0.0f);
    if (inPlace != 0)
        VU1_addGSregister(0x47, 0);
    else
        VU1_addGSregister(0x47, 0x30000);
    VU1_addGSregister(VU1_SCREEN_ALPHA_GS_REG, OCCL_DEBUG_END_ALPHA);
}

void PutDrawBufferLarge();
void InitViewContext();
void UpdateViewContext();

void draw_prepareFrame(void) {
    PutDrawBufferLarge();
    InitViewContext();
    UpdateViewContext();
}

// Per-frame debug font stage, run from DrawDebugProfiler for the 0x20 stage
// bit between SetupGifPaging(1) and DoGifPaging(). When glyph quad records
// are pending (fontQuadCount, filled by level overlays), programs the font
// GS registers, prepares the records (sphere clip, camera-space transform,
// depth alpha, texture cursor), queues the font VU state with a temporary
// depth bias, draws the glyph quads, then resets the bias and the last
// GS register. The GS register numbers are the original's values; their
// hardware semantics are undocumented for this range, so they stay literal.
#define DEBUG_FONT_DEPTH_BIAS -0.04f

extern int fontQuadCount;
extern "C" void prepareFontQuads(void);
extern "C" void drawFontQuads(void);

extern "C" void drawDebugFont(void) {
    if (fontQuadCount != 0) {
        VU1_addGSregister(8, 5);
        VU1_addGSregister(0x14, 0x61);
        VU1_addGSregister(0x47, 0x513F1);
        VU1_addGSregister(0x4A, 1);
        prepareFontQuads();
        fontDepthBiasGp = DEBUG_FONT_DEPTH_BIAS;
        FontQueueVUState();
        drawFontQuads();
        // The bias reset is an int zero store (sw $0), not a float store.
        *(int*)&fontDepthBiasGp = 0;
        VU1_addGSregister(0x4A, 0);
    }
}

extern u32 bitSwapLut[256] __attribute__((section(".data")));

void buildBitSwapLut(void) {
    for (int i = 0; i < 256; i++) {
        int index = i & 0xE7;
        if (i & 0x8) index |= 0x10;
        if (i & 0x10) index |= 0x8;
        bitSwapLut[index] = (u32)((i >> 1) << 24);
    }
}

// Unreachable dead tail the original compiler emitted after
// buildBitSwapLut: an alignment nop, two stack deallocations
// (0x80 and 0x170, the frame of a function no longer in the source),
// plus the alignment nop before FastIntersectVert in drawquad.o.
// EGC 2.95.2 never regenerates dead frame deallocations after the
// epilogue (probed; see decomp_state/notes/989snd_func_0012E078.md),
// so the bytes are preserved with raw asm.
asm("nop");
asm("addiu $sp,$sp,0x80");
asm("nop");
asm("addiu $sp,$sp,0x170");
asm("nop");
