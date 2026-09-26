# draw_post_post func_001F2260 (UpdateDrawCamera__Fv) — BLOCKED

vram 0x1F2260, 0x328 bytes / 202 instr, `UpdateDrawCamera__Fv` (no params).
Source placeholder: `code/game/draw_post_post.cpp:6` (INCLUDE_ASM).
Reference: `code/_generated/nonmatchings/game/draw_post_post/func_001F2260.s`.

Status: **blocked** (EGC 2.95.2 body register-allocation wall, "Wall B").
Semantics, data layout, ABI, and the full prologue are verified and correct;
the prologue is reproduced byte-for-byte (see Wall A fix below). The body
(≈174 word-diffs of 202, only ~28 words match) cannot be reproduced from C
with this EGC build after 15+ source variants, both scheduler flags, a bit-cast
probe, and both required escalations (one-shot `expert` + `last-resort-decompiler`).

## Verified semantics

Rebuilds `currentCamera`'s basis (`mtx0`) and its derived matrices, and emits
the "sky view matrix" (`skyViewMtx`).

```
void UpdateDrawCamera() {
    CameraMatrix rotMtx; /* stack, sp+0 */
    if (occlCamStaged) {                 /* guard flag, prologue bnez */
        camPolarInit(&rotMtx);                                   // 0x125218
        camPolarRot0(&rotMtx,&rotMtx, currentCamera.rot[0]);     // 0x125360, angle=cam+0x150
        camPolarRot1(&rotMtx,&rotMtx, currentCamera.rot[1]);     // 0x125408, angle=cam+0x154
        camPolarRot2(&rotMtx,&rotMtx, currentCamera.rot[2]);     // 0x1252B8, angle=cam+0x158
    } else {
        rotMtx = *stagedOrient;       /* 0x187290, 3×8-byte staged quads */
    }
    // build currentCamera.mtx0 basis from rotMtx (1.0f const @0x3F800000, neg.s,
    //   swc1 f0->cam+0x3C, sw 0->cam+0x30, ...); then compute the derived 4x4
    //   slots from the view context via draw_transformMatrix (x4) and
    //   draw_scaleQuad (x2) — the 12-statement hvdf row-blend block peaks at 24
    //   live FP values (exactly f20-f23 for the products).

    // FINAL: skyViewMtx (0x18CF80) = camera view matrix
    skyViewMtx[0..2] = currentCamera.mtx0[0..2];   /* 3× lq/sq, 48 bytes */
    float scale = 1024.0f;                         /* lui 0x44800000; mtc1 f20 — LATE */
    FastVecScale(&skyViewMtx.q3, &currentCamera.pos /*+0x140*/, scale); // f12=f20 in jal delay slot
    skyViewMtx.q3.w = scale;                       /* swc1 f20, 0x3C(skyViewMtx) */
}
```

The polar angles are `currentCamera.rot` (PolarSm @ 0x150: azimuth/elevation/
radius decomposed from orientMtx). `camPolar*` are in-place VU builders
(dst==src) that rotate the 64-byte matrix about one polar axis; `camPolarInit`
fills the quads with (0,0,0,1) first.

## Verified data layout

- `currentCamera` = 0x186F40 (s0 throughout). pos @ +0x140, rot(PolarSm) @ +0x150.
  (drawCamera = 0x18CF10 is a separate slot; this function drives currentCamera.)
- `viewCtx` = 0x18CD00: field_40@+0x40, fMtx@+0xC0, nfMtx@+0x100, hMtx@+0x140,
  hvdf[4]@+0x1A0, guardX@+0x1C0. (Named in config/symbols.txt; struct in camera.h.)
- `fMtx` = viewCtx.fMtx = 0x18CDC0 (s1, fresh lui, cluster primary).
- `stagedOrient` = 0x187290 (the else-path source).
- `skyViewMtx` = 0x18CF80 (s1 RELOADED at the tail).
- `occlCamStaged` = prologue guard (bnez target selects polar vs staged path).

Callees (aliases in config/linker_aliases.ld): camPolarInit=0x125218,
camPolarRot0=0x125360, camPolarRot1=0x125408, camPolarRot2=0x1252B8,
draw_scaleQuad=0x1F9A80, draw_transformMatrix=0x1FA378, FastVecScale=0x1F9A68.

## Wall A (frame / FP-save count) — CRACKED

Original prologue: frame 0xC0 (192); saves ra,s4..s0 (sq @144..64) + f23,f22,f21,
then `bnez` with **f20 saved in the delay slot** → exactly 4 FP saves (f20-f23).
EGC hoisted the 1024.0f constant early into a 5th saved FP reg (f24), growing the
frame to 0xD0 and shifting every sp-relative offset (the dominant diff cascade).

Fix (proven idiom, cf. `Camera_Pos2Polar3d`, code/game/camera.cpp:348
`register float pihalf asm("$f21");`): pin the scale to f20 with an
**uninitialized** hard-FPR pin + a zero-byte operand-free barrier BEFORE the
assignment, so the `li.s f20,1024.0f` stays LATE (reusing f20 after the 24-live
row-add products are dead):

```cpp
register float scale asm("$f20");
asm volatile("");
scale = 1024.0f;
FastVecScale(row3, &cam->pos, scale);
row3->w = scale;
```

Result: frame 192, fmask 0x00f00000 (f20-f23 only, no f24), constant late —
prologue matches byte-for-byte. An initialized pin (`= 1024.0f`) or bare literal
hoists the load early; an integer bit-cast kills f24 but adds an s5 GPR save.

## Wall B (body register allocation) — the BLOCKER

Even with the prologue matched (best probes vM6/vM7), the body has ~174
word-diffs (only ~28 of 202 words match). Region breakdown of the best probe
(vM6): mid(call3/scale/copy)=54, final-block=54, join=30, else-branch=15,
row-add=12, if-branch=9. Concrete residuals:

- **hi(cam) at the join lives in v0($2) in the original** (both branches reload
  it: the if-branch in the `b` delay slot, the else-branch as a fresh `lui` AFTER
  the last `jal camPolarRot2`); the candidate keeps it in v1($6), changing the
  join store base and the cam recompute.
- **s3/s4 swapped**: candidate s3=m4, s4=hMtx vs original s3=hMtx, s4=m4.
- **row-add FP product register assignments** differ (the 24-live block maps to
  different f-registers than the original).
- **if-branch structure/order** differs (stagedOrient copy vs polar call sequence
  landing on different branch targets), and the **quad-copy interleave** +
  **else-branch join scheduling** differ, cascading into the final block.

Exhausted routes: source anchoring variants vD-vK (viewCtx-cluster primary vs
fMtx primary vs direct), 12-statement vs other row-add forms, both scheduler
flags (`-fno-schedule-insns` → frame 176/1 FP save; `-fno-schedule-insns2` → no
effect), integer bit-cast, the f20-pin+barrier (Wall A). EGC 2.95.2
(`-G8 -O2 -ffast-math -fno-exceptions -snas`) does not reproduce the original
body allocation from C. This is the same class of callee-saved/FP-allocation
wall as effects_func_001EDC50.

## Escalation summary (required before blocking)

- **expert** (one-shot GPT-6 Astra): confirmed the constant is 1024.0f (not the
  earlier 2.0f misread) and proposed the integer bit-cast to reuse an FP reg —
  tried (vM3), killed f24 but added an s5 GPR save, net worse.
- **last-resort-decompiler** (GPT-5.6 Sol): recommended the uninitialized hard-FPR
  pin idiom (`register float scale asm("$f20"); scale = 1024.0f; ...`) with an
  operand-free `asm volatile("")` barrier before the assignment. Applied (vM6/vM7):
  cracked Wall A (frame 192, 4 FP saves, prologue matches) but did not resolve
  Wall B. **last-resort GPT-5.6 Sol used.**

## Probe repro

```
source .venv/bin/activate
python3 tools/decomp_probe.py <variant> \
  code/_generated/nonmatchings/game/draw_post_post/func_001F2260.s \
  UpdateDrawCamera__Fv \
  --define vcFmtx=0x18CDC0 --define stagedOrient=0x187290 \
  --define camPolarInit=0x125218 --define camPolarRot0=0x125360 \
  --define camPolarRot1=0x125408 --define camPolarRot2=0x1252B8 \
  --define draw_transformMatrix=0x1FA378 --define draw_scaleQuad=0x1F9A80 \
  --define FastVecScale=0x1F9A68 --out working/draw_post_post_func_001F2260/probe_vX
```
Splat/candidate.json hex words are byte-reversed LE; decode by reversing bytes
before bit-field extraction.

Best candidates (prologue matched, body Wall B): variantM6_barrier.cpp,
variantM7_derived.cpp (adds `ViewCtx* ctx = (ViewCtx*)((char*)&vcFmtx - 0xC0);`
so viewCtx is derived, matching the original `addu s2,s1,-0xC0`).
