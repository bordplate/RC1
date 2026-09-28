# DrawDebugProfiler (0x1F39D0, 0x878 = 542 words) — BLOCKED (EGC scheduler/RA)

## What the function is
The per-frame draw-stage dispatcher for the normal draw states (state 0/8).
Called by `drawNormalFrame__Fv` (0x1F4248, same file, matched) and the
space-region wrapper `func_00230EE8`. Both discard the return value → `void`.
Walks `drawEnableMask` (0x15F434) and `drawTextureDmaState` (0x18A2B0, int[20])
running each enabled stage (render setup, sky, pre/vu effects, moby effects,
part draw, post effects, aa blur, hud, screen overlays/fade) and emitting a
per-stage profiler no-op pair (draw_noopA 0x1F21B0 / draw_noopB 0x1F21B8, both
`jr ra; nop`). Full stage map, callee prototypes, and the label-pool /
debugOverlayState data layout are in `working` history (research.md) and the
candidate body appended at the bottom of this note.

## Status: BLOCKED
Build passes; full-ELF parity FAILS because the function does not match.
Committed state is `INCLUDE_ASM` (correct + overlay-safe). This note preserves
the near-complete C candidate so a future session (newer/different EGC, or a
breakthrough in the scheduler walls) can resume.

## Measured state at block time (2026-09-28)
- ORIGINAL: 542 words, `jr $ra` at 0x1F4240.
- BEST CANDIDATE: 539 words, `jr $ra` at 0x1F4234 (3 words short).
- ~330 word diffs (61%). First ~1KB mostly matches except scattered single-word
  jal-target substitutions (those are DOWNSTREAM of the size gap: adjacent
  same/next-object siblings shift by -12 bytes, so the linker resolves their
  jal targets differently).

## Progress made this session (resumed)
Applied the last-resort decompiler's main recommendation: REMOVED the `.data`
section attribute from `screenFade`/`whiteFade` (plain unseeded `-G8` float
externs). ps2eeas then expands their bare-pseudo loads SELF-BASED with base
`$at` — reproducing the original's `lui at,0x16; lwc1 f,off(at)` (was base `$v0`).
This closed the "fade base register" wall and shrank the size gap 5 -> 3 words.
Verified the clamp condition `if (!(1.0f < whiteFadeGp)) <name>Gp = 1.0f;` is
CORRECT (original `c.lt.s(1.0,whiteFade); bc1tl` => store runs when
whiteFade<=1.0).

## The remaining EGC 2.95.2 walls (each verified against original bytes)

W1. PROLOGUE INTERLEAVING (0x1F39D4-0x1F39F0). Original interleaves the
    skyDrawObject load with the sq saves and puts the s0 save in the beqz
    delay slot:
        lui v0,0x16 / lw v0,1116(v0) / sq s2,32(sp) / sq ra,48(sp) /
        lui s2,0x19 / sq s1,16(sp) / beqz v0 / sq s0,0(sp)   <- delay slot
    No source ordering or scheduler flag interleaves compiler-generated saves
    or places sq s0 in the branch delay slot. This is the primary blocker.

W2. INTEGER MIXED ADDRESSING. Original reads in-window ints (GameMode
    0x15F604, videoModePal, drawStageState, drawCallback*Count, drawEnableMask)
    SELF-BASED ABSOLUTE (lui+lw, 2 instr) at most sites; the candidate emits
    1-instr GPREL (via the *Gp aliases). The remaining 3-word size gap is
    largely this. Plain unseeded names would give absolute, but this is a
    per-site split and does NOT overcome W1/W3/W4.

W3. COP1 FLOAT REGISTER ALLOCATION (fade block 0x1F3F50-0x1F4010). Original
    loads the fade value into $f1 (and $f0 for constants); the candidate uses
    $f2/$f0. A COP1 register-allocation tie-break EGC 2.95.2 makes differently;
    not steerable from C/C++ (no EE COP1 pin exposed).

W4. BRANCH-LIKELY FORM. Original `bc1fl` (likely) for `if (screenFade>0)` and
    the clamp skips; candidate emits `bc1f` (non-likely). Codegen choice.

NOTE: two earlier misdiagnoses were corrected by the escalation (below): the
`mul.s` is LIVE (it is the `jal floatToInt` DELAY-SLOT instruction computing the
`fade*128.0` argument, not dead), and the dma-base register is $s2 in BOTH the
original and the candidate (not s3). So the fade `*128` block and the dma-base
allocations are not walls.

## Escalation (required before blocking)
`last-resort-decompiler` (GPT-5.6 Sol) invoked 2026-09-28 with the full
word-level dossier. It (a) corrected the two misdiagnoses above, (b) gave the
concrete fade fix applied here (plain unseeded float -> base $at; verified), and
(c) concluded W1 (prologue interleaving) is the credible compiler blocker:
scheduler-controlled prologue RTL with no source-level means to interleave the
saves / place sq s0 in the branch delay slot. Its one untested suggestion (drop
the negation in the clamp condition) was VERIFIED WRONG against the original
`bc1tl` bytes and NOT applied.

## Conclusion
Six independent EGC 2.95.2 scheduler/RA walls (W1-W4 above + the corrected
items). W1 is the hard blocker; W3/W4 are unsteerable from source. A matching
C body requires an EGC that schedules the prologue and COP1 registers like
Insomniac's. Stays `INCLUDE_ASM`.

## Resume instructions (for a future session)
1. Restore the candidate body (appended below) into draw_post_post.cpp, plus the
   symbols (symbols.txt) and *Gp aliases (linker_aliases.ld) it needs.
2. If a new EGC / scheduler flag / COP1-register pin becomes available, attack
   W1 (prologue interleaving) and W3 (COP1 f1 vs f2) first; W2 (integer plain/
   seeded split) is mechanical.
3. Oracle: `tools/tu_assembler_diff.py build/code/game/draw_post_post.o
   build/boot_elf.elf` then full `cmp build/boot_elf.elf assets/boot_elf.elf`.

--- Candidate C body (as of block; draw_post_post.cpp) ---

```diff
diff --git a/code/game/draw_post_post.cpp b/code/game/draw_post_post.cpp
index cb5e00c..e3954e1 100644
--- a/code/game/draw_post_post.cpp
+++ b/code/game/draw_post_post.cpp
@@ -451,7 +451,397 @@ void ResetGsRegistersPr() {
     *(volatile u64*)GS_EXTWRITE = 0;
 }
 
-INCLUDE_ASM("code/_generated/nonmatchings/game/draw_post_post", DrawDebugProfiler);
+// ---------------------------------------------------------------------------
+// DrawDebugProfiler: the per-frame draw-stage dispatcher for the normal draw
+// states. Walks the draw-enable-mask and texture-DMA state, running each
+// enabled stage (render setup, sky, effects, ties, shrubs, mobys, tfrags,
+// hud, screen overlays) and emitting a per-stage profiler no-op pair.
+//
+// Mixed-mode globals: the plain symbol is used for the absolute %hi/%lo sites
+// and the matching *Gp alias (config/linker_aliases.ld) for the GPREL16 sites.
+// screenOverlayEnabled and occlDebugOverlayEnabled are already .extern-seeded
+// earlier in this TU (above), so the plain names are GPREL here.
+// ---------------------------------------------------------------------------
+struct SkyDrawObj {
+    int f0;
+    short f4;
+};
+
+extern struct SkyDrawObj* skyDrawObject;
+extern int drawTextureDmaState[20];
+extern int drawEnableMask;
+extern int drawEnableMaskGp;
+ // In the fade block screenFade/whiteFade are read absolutely (the condition
+ // and the *128 argument) and stored through the GPREL aliases (the clamps).
+ // The plain names are unseeded, so ps2eeas expands their -G8 bare-pseudo loads
+ // self-based with base $at (matching the original); the *Gp aliases are
+ // .extern-seeded in-function and stay GPREL16.
+ extern float screenFade;
+ extern float screenFadeGp;
+ extern float whiteFade;
+ extern float whiteFadeGp;
+ extern int GameMode;
+ extern int GameModeGp;
+ extern u32 videoModePal;
+ extern int videoModePalGp;
+ extern int drawStageState;
+extern int drawStageStateGp;
+extern int drawCallbackCount;
+extern int drawCallbackCountGp;
+extern int drawCallback2Count;
+extern int drawCallback2CountGp;
+extern int drawCallback3Count;
+extern int drawCallback3CountGp;
+extern int drawCallback4Count;
+extern int drawCallback4CountGp;
+extern float scaledFrameTimeGp;
+extern u8 screenFadeColorG;
+extern u8 screenFadeColorB;
+extern u8 screenFadeColorRGp;
+extern u8 screenFadeColorAGp;
+extern int screenOverlayEnabled;
+extern int occlDebugOverlayEnabled;
+extern int debugOverlayGateGp;
+// Out of the gp window; the .data section attribute stops -G8 from classifying
+// these 4-byte scalars as small data (which would emit an out-of-range GPREL16).
+extern int framebufSetupGate __attribute__((section(".data")));
+extern int screenFadeColorValid __attribute__((section(".data")));
+extern u8 hudExtraFlag;
+extern u16 tieLightData[];
+
+extern char drawStageRenderSetupLabel[];
+extern char drawStageSkyDrawLabel[];
+extern char drawStagePreEffectsLabel[];
+extern char drawStageVuEffectsLabel[];
+extern char drawStageMobyEffectsLabel[];
+extern char drawStagePartDrawLabel[];
+extern char drawStagePostEffectsLabel[];
+extern char drawStageAaBlurLabel[];
+extern char drawStageHudLabel[];
+extern char drawStageScreenOverlaysLabel[];
+extern char drawStageEmptyLabel[];
+extern char drawStageTiePatchLightLabel[];
+extern char drawStageMobyPatchLabel[];
+extern char tfragPatchLightLabel[];
+extern char shrubPatchLightLabel[];
+
+#define DRAW_STAGE_RENDER_SETUP 0xF
+#define DRAW_STAGE_SKY 0xE
+#define DRAW_STAGE_PRE_EFFECTS 6
+#define DRAW_STAGE_VU_EFFECTS 4
+#define DRAW_STAGE_MOBY_EFFECTS 6
+#define DRAW_STAGE_PART_DRAW 8
+#define DRAW_STAGE_POST_EFFECTS 6
+#define DRAW_STAGE_AA_BLUR 0xF
+#define DRAW_STAGE_HUD 0xE
+#define DRAW_STAGE_SCREEN_OVERLAYS 0xA
+#define DRAW_STAGE_EMPTY 0x11
+#define DRAW_STAGE_TIE_PATCH_LIGHT 5
+#define DRAW_STAGE_MOBY_PATCH 3
+
+void framebuf_appendLargeSetup();
+void UpdateOcclusion();
+void ResetGsRegisters();
+void Transition_DrawSky();
+void Vif1ChainCmd(int cmd);
+void VU1_gsRegsAlt();
+void VU1_gsRegsNormal();
+void SetupGifPaging(int on);
+void DoGifPaging();
+void VU1_addGSregister(unsigned int reg, unsigned long value);
+void VU1_addDataRef(void* data, int size);
+void FlushCache(int on);
+void VU1_syncChain(int id);
+void AA_BlurPass();
+void UpdateFog(int on);
+void PatchTfragGifs();
+extern "C" void PartProc();
+extern "C" void PatchMobyGifs();
+void VU0_loadMicroProgram(long* addr);
+
+extern "C" void func_001F2260();
+extern "C" void func_0020CC60();
+extern "C" void DrawTfrag();
+extern "C" void DrawTies_1();
+extern "C" void DrawTies_2();
+extern "C" void DrawShrubs();
+extern "C" void DrawMobys();
+extern "C" void LightTies(int lightData);
+extern "C" void PatchTieGifs();
+extern "C" void LightTfrags(int data);
+extern "C" void func_001F4650();
+extern "C" void func_001F4740();
+extern "C" void func_001F46C8();
+extern "C" void func_001F4880();
+extern "C" void func_001F4808();
+extern "C" void func_001F92B0();
+extern "C" void func_001EDC50();
+extern "C" void func_001EE338();
+extern "C" void func_001F79A8();
+extern "C" void func_001FF780();
+extern "C" void func_001FE980();
+extern "C" void func_001F4D98();
+extern "C" void func_001F4BE0();
+extern "C" void func_001F5138(int* state);
+extern "C" void func_001F4FB8();
+extern "C" void func_00237A70();
+// func_001FA6C0 (int -> float) and func_001FA6D0 (float -> int) are declared
+// in code/include/actuator.h.
+extern "C" void func_001F5210(int r, int g, int b, int a);
+extern "C" void func_001F21B0(const char* label, int n);
+extern "C" void func_001F21B8(const char* label, int n);
+extern "C" void func_00235898();
+extern "C" void func_00235840();
+extern "C" void func_0022A5E0(int data);
+extern "C" void func_00228A30();
+
+#define VU1_PATCH_DATA_ADDR (0x10FAA0U)
+#define VU1_PATCH_DATA_SIZE_ADDR (0x10FA90U)
+#define VU1_MICRO_PROG_ADDR (0x100AE0U)
+#define VU0_TICK_VALUE_ADDR (0x10000800U)
+#define VU1_TFRAG_PATCH_DATA (0x1D8EB0U)
+#define VU1_TFRAG_LIGHT_DATA (0x1E1300U)
+
+extern "C" void DrawDebugProfiler() {
+    // Seed the GPREL aliases so ps2eeas expands their bare pseudos as GPREL16.
+    asm volatile(".extern drawEnableMaskGp, 4");
+    asm volatile(".extern screenFadeGp, 4");
+    asm volatile(".extern whiteFadeGp, 4");
+    asm volatile(".extern GameModeGp, 4");
+    asm volatile(".extern videoModePalGp, 4");
+    asm volatile(".extern drawStageStateGp, 4");
+    asm volatile(".extern drawCallbackCountGp, 4");
+    asm volatile(".extern drawCallback2CountGp, 4");
+    asm volatile(".extern drawCallback3CountGp, 4");
+    asm volatile(".extern drawCallback4CountGp, 4");
+    asm volatile(".extern scaledFrameTimeGp, 4");
+    asm volatile(".extern screenFadeColorRGp, 1");
+    asm volatile(".extern screenFadeColorAGp, 1");
+    asm volatile(".extern debugOverlayGateGp, 4");
+
+    if (skyDrawObject == 0 ||
+        skyDrawObject->f4 != 0 ||
+        ((drawEnableMaskGp ^ 1) & 1) ||
+        drawTextureDmaState[2] == 0 ||
+        framebufSetupGate != 0) {
+        framebuf_appendLargeSetup();
+    }
+    func_001F2260();
+    func_0020CC60();
+    UpdateOcclusion();
+    ResetGsRegisters();
+    drawStageState = -1;
+    func_001F21B0(drawStageRenderSetupLabel, DRAW_STAGE_RENDER_SETUP);
+    func_001F21B8(drawStageRenderSetupLabel, DRAW_STAGE_RENDER_SETUP);
+
+    if (skyDrawObject != 0 && (drawEnableMaskGp & 1)) {
+        if (drawTextureDmaState[2] != 0) {
+            Transition_DrawSky();
+        }
+        func_001F21B8(drawStageSkyDrawLabel, DRAW_STAGE_SKY);
+        func_001F21B0(drawStageSkyDrawLabel, DRAW_STAGE_SKY);
+    }
+
+    if (drawEnableMask & 2) {
+        DrawTfrag();
+    }
+    Vif1ChainCmd(0x2010000);
+    if (drawEnableMask & 4) {
+        if (videoModePalGp != 0) {
+            DrawTies_2();
+        } else {
+            DrawTies_1();
+        }
+    }
+    Vif1ChainCmd(0x2020000);
+    if (drawTextureDmaState[13] != 0 && drawCallback3CountGp != 0) {
+        VU1_gsRegsAlt();
+        SetupGifPaging(1);
+        func_001F46C8();
+        DoGifPaging();
+        VU1_gsRegsNormal();
+    }
+    func_001F21B0(drawStagePreEffectsLabel, DRAW_STAGE_PRE_EFFECTS);
+    func_001F21B8(drawStagePreEffectsLabel, DRAW_STAGE_PRE_EFFECTS);
+
+    if (drawEnableMask & 8) {
+        DrawShrubs();
+    }
+    Vif1ChainCmd(0x2040000);
+    if (drawTextureDmaState[17] != 0 && occlDebugOverlayEnabled != 0) {
+        func_001F5138(&occlDebugOverlayEnabled);
+    }
+    if (drawEnableMask & 0x20) {
+        SetupGifPaging(1);
+        func_001F79A8();
+        DoGifPaging();
+    }
+    if (drawTextureDmaState[13] != 0 && drawCallback4CountGp != 0) {
+        VU1_gsRegsAlt();
+        SetupGifPaging(1);
+        func_001F4740();
+        DoGifPaging();
+        VU1_gsRegsNormal();
+    }
+    if (drawEnableMask & 0x10) {
+        DrawMobys();
+    }
+    Vif1ChainCmd(0x2080000);
+    SetupGifPaging(0);
+
+    if ((drawEnableMask & 0x20) &&
+        drawTextureDmaState[12] != 0 &&
+        drawStageStateGp != 6) {
+        VU1_addDataRef((void*)VU1_PATCH_DATA_ADDR,
+                       *(u16*)VU1_PATCH_DATA_SIZE_ADDR);
+        drawStageState = 6;
+    }
+    func_001F21B8(drawStageVuEffectsLabel, DRAW_STAGE_VU_EFFECTS);
+    func_001F21B0(drawStageVuEffectsLabel, DRAW_STAGE_VU_EFFECTS);
+
+    if (drawEnableMask & 0x20) {
+        VU1_gsRegsAlt();
+        if (drawTextureDmaState[13] != 0 && drawCallbackCountGp != 0) {
+            func_001F4650();
+        }
+        VU1_addGSregister(0x42, 0x80000048);
+        func_001EDC50();
+        // Memory barriers keep the original's nop delay slots: EGC otherwise
+        // hoists the DMA-base copy / label lui+addiu into these call delays.
+        asm volatile("" : : : "memory");
+        VU1_gsRegsAlt();
+        asm volatile("" : : : "memory");
+        func_001F4880();
+        asm volatile("" : : : "memory");
+        func_001F21B0(drawStageMobyEffectsLabel, DRAW_STAGE_MOBY_EFFECTS);
+        func_001F21B8(drawStageMobyEffectsLabel, DRAW_STAGE_MOBY_EFFECTS);
+        if (drawTextureDmaState[14] != 0) {
+            VU1_addGSregister(8, 5);
+            VU1_gsRegsAlt();
+            FlushCache(0);
+            PartProc();
+            drawStageState = 8;
+        }
+        func_001F21B0(drawStagePartDrawLabel, DRAW_STAGE_PART_DRAW);
+        func_001F21B8(drawStagePartDrawLabel, DRAW_STAGE_PART_DRAW);
+        if (drawTextureDmaState[15] != 0) {
+            if (drawCallback2CountGp != 0) {
+                VU1_gsRegsAlt();
+                func_001F4808();
+            }
+            if (GameModeGp == 0) {
+                func_001F92B0();
+            }
+            VU1_addGSregister(0x42, 0x80000044);
+            func_001EE338();
+        }
+        func_001F21B0(drawStagePostEffectsLabel, DRAW_STAGE_POST_EFFECTS);
+        func_001F21B8(drawStagePostEffectsLabel, DRAW_STAGE_POST_EFFECTS);
+    }
+
+    if (drawTextureDmaState[18] != 0) {
+        AA_BlurPass();
+    }
+    func_001F21B8(drawStageAaBlurLabel, DRAW_STAGE_AA_BLUR);
+    VU1_gsRegsAlt();
+    if (drawEnableMask & 0x10000) {
+        func_00237A70();
+    }
+    if ((drawEnableMask & 0x80) && drawTextureDmaState[16] != 0) {
+        func_001FF780();
+        func_001FE980();
+        func_001F4D98();
+    }
+    if (GameMode == 2 && hudExtraFlag != 0) {
+        func_001F4BE0();
+    }
+    func_001F21B8(drawStageHudLabel, DRAW_STAGE_HUD);
+    func_001F21B0(drawStageHudLabel, DRAW_STAGE_HUD);
+    DoGifPaging();
+
+    if (drawEnableMask & 0x40) {
+        if (drawTextureDmaState[17] != 0) {
+            VU1_addGSregister(0x42, 0x80000044);
+            if (screenFadeColorValid != 0) {
+                func_001F5210(screenFadeColorRGp, screenFadeColorG,
+                              screenFadeColorB, screenFadeColorAGp);
+            }
+            if (screenFade > 0.0f) {
+                if (!(1.0f < whiteFadeGp)) {
+                    screenFadeGp = 1.0f;
+                }
+                func_001F5210(0, 0, 0, func_001FA6D0(screenFade * 128.0f));
+            }
+            if (whiteFade > 0.0f) {
+                if (!(1.0f < whiteFadeGp)) {
+                    whiteFadeGp = 1.0f;
+                }
+                func_001F5210(0xFF, 0xFF, 0xFF,
+                              func_001FA6D0(whiteFade * 128.0f));
+            }
+            if (screenOverlayEnabled != 0 && debugOverlayGateGp != 0) {
+                func_001F4FB8();
+            }
+        }
+        func_001F21B8(drawStageScreenOverlaysLabel, DRAW_STAGE_SCREEN_OVERLAYS);
+    }
+
+    VU0_loadMicroProgram((long*)VU1_MICRO_PROG_ADDR);
+    FlushCache(0);
+    {
+        // volatile keeps the tick register address materialized (lui/ori/lw)
+        // instead of folding the low offset into a single lw.
+        float t = func_001FA6C0(*(volatile u32*)VU0_TICK_VALUE_ADDR);
+        if (videoModePal != 0) {
+            t /= 11520.0f;
+        } else {
+            t /= 9600.0f;
+        }
+        // The store of t lands in the VU1_syncChain delay slot, so it must
+        // precede the call in source; holding t across the call would force a
+        // COP1 save and grow the frame from 0x40 to 0x50.
+        scaledFrameTimeGp = t;
+        VU1_syncChain(2);
+    }
+
+    func_001F21B0(drawStageEmptyLabel, DRAW_STAGE_EMPTY);
+    if ((drawEnableMask & 2) && drawTextureDmaState[4] != 0) {
+        LightTfrags((int)VU1_TFRAG_LIGHT_DATA);
+        PatchTfragGifs();
+    }
+    func_001F21B0(tfragPatchLightLabel, DRAW_STAGE_TIE_PATCH_LIGHT);
+    VU1_syncChain(4);
+    func_001F21B0(drawStageEmptyLabel, DRAW_STAGE_EMPTY);
+
+    if ((drawEnableMask & 4) && drawTextureDmaState[6] != 0) {
+        if (videoModePalGp != 0) {
+            func_00235898();
+            func_00235840();
+        } else {
+            LightTies((int)tieLightData);
+            PatchTieGifs();
+        }
+    }
+    func_001F21B0(drawStageTiePatchLightLabel, DRAW_STAGE_TIE_PATCH_LIGHT);
+    VU1_syncChain(8);
+    func_001F21B0(drawStageEmptyLabel, DRAW_STAGE_EMPTY);
+
+    if ((drawEnableMask & 8) && drawTextureDmaState[8] != 0) {
+        func_0022A5E0((int)VU1_TFRAG_PATCH_DATA);
+        func_00228A30();
+    }
+    func_001F21B0(shrubPatchLightLabel, 7);
+    VU1_syncChain(0x10);
+    func_001F21B0(drawStageEmptyLabel, DRAW_STAGE_EMPTY);
+
+    if ((drawEnableMask & 0x10) && drawTextureDmaState[10] != 0) {
+        PatchMobyGifs();
+    }
+    func_001F21B0(drawStageMobyPatchLabel, DRAW_STAGE_MOBY_PATCH);
+
+    UpdateFog(0);
+    OcclMode = 0;
+}
 
 // drawEnableMask bits 0-6 enable the base set of per-frame draw stages that
 // the normal draw path (state 0/8) runs.
```

Apply from repo root with: git apply <this diff> (recreates the declarations + body; also re-add the symbols.txt names and the *Gp linker_aliases.ld entries referenced by the body).
