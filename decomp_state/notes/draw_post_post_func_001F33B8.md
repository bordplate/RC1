# func_001F33B8 — `SetUserViewContext` (BLOCKED: EGC prologue interleaving tiebreak)

- Source: `code/game/draw_post_post.cpp:306` (`INCLUDE_ASM(..., func_001F33B8)`)
- Generated asm: `code/_generated/nonmatchings/game/draw_post_post/func_001F33B8.s`
- vram `0x1F33B8`, 304 bytes (76 words). Callers: `func_001F7888`
  (draw_post_post.cpp:625), `func_00239690` (code/game/vendor.cpp:23).
- Named `SetUserViewContext` after Deadlocked PAL
  `reference/dl/game_dl/draw.cpp:1810` (`SetUserViewContext(int width, int height,
  float xratio, float fogNearDist, float fogFarDist, float fogNearIntensity,
  float fogFarIntensity)`), mangled `func_001F33B8` (unmangled C linkage in the
  boot ELF). It is the user/occlusion-path sibling of the matched
  `InitViewContext` (~line 240) which shares the same occlViewParams and viewCtx
  writes.

## Semantics (verified)
Writes the occlusion view-params and the view-context clip/fog/pixel-scale
fields, then calls `UpdateViewContext()`:
- `occlViewParams` (0x13E500): paramX=width, paramY=height, halfX=width>>1,
  halfY=height>>1, minX=(0x800-halfX)<<4, minY=(0x800-halfY)<<4,
  maxX=(halfX+0x800)<<4, maxY=(halfY+0x800)<<4.
- `viewCtx`: nearClip=32.0f, farClip=524288.0f (0x49000000 — the binary literal,
  NOT `VIEWCTX_FAR_CLIP` 745472.0f), xratio=xratio,
  xpix=cvt.s.w(width)*0.5f, ypix=cvt.s.w(height)*0.5f,
  xclip=xpix*4.0f, yclip=ypix*4.0f, fogNearDist/fogFarDist/fogNearIntensity/
  fogFarIntensity = the four float args.

ABI (probe-verified, `working/.../abi_probe.c`): 2 int args in a0/a1, 5 float
args DENSE-packed f12..f16 (f12=xratio, f13=fogNearDist, f14=fogFarDist,
f15=fogNearIntensity, f16=fogFarIntensity). `func_001FA6C0` = cvt.s.w
(actuator.h:25, `extern "C" float func_001FA6C0(int);`).

## Verified constraints (reproduce the TAIL exactly)
Best candidate = **cand16** (working/draw_post_post_func_001F33B8/cand16.cpp),
304 bytes. It requires:
- Fog values pinned to callee-saved FPRs: `register float fogND asm("$f22")=
  fogNearDist; fogFD "$f23"; fogNI "$f24"; fogFI "$f21";` — produces the five
  prologue FPU saves (swc1 f24..f20 to 80/72/64/56/48(sp), f20=halfScale) and
  the correct `mov.s` fog copy rotation (f22=f13, f23=f14, f24=f15, f21=f16).
- yclip computed out-of-place into f3: `register float yclipVal asm("$f3") =
  yScale * 4.0f;` (keeps yScale in f0).
- **The tail fix:** `asm volatile("" : "+f"(yScale));` immediately before
  `viewCtx.ypix = yScale;`. The FPU `"f"` constraint (NOT `"r"`) keeps yScale
  (f0) live so the ypix store lands in the `jal` delay slot. A tied `"+r"` or
  input `"r"` barrier on the FPU value emits an `mfc1/mtc1` GPR round-trip
  (+2 words); `asm volatile("" ::: "memory")` is a cfront PARSE ERROR (no clobber
  lists in this EGC).

With these, the entire tail (from instruction ~45 on), the occlViewParams base
split (lui t3,0x14 / addiu v0,t3,-6912), the cvt.s.w body, and the xpix/ypix/
xclip/yclip/fog stores all match the linked original. Only the PROLOGUE
(35 words, linked 0x1f33d4..0x1f3468, instructions 8-45) differs.

## The remaining prologue mismatch (unmatchable tiebreak)
- cand16: `sq ra` at instruction 17; the five FPU prologue saves are GROUPED
  (8,10,11,13,15); the viewCtx base (lui s0; addiu s0) is loaded LATE (18-19);
  the three min/max `sll` are GROUPED (29-31); the four fog `mov.s` are GROUPED
  after the two `lui at/mtc1` float-constant pairs (34-40).
- original: the six-instruction setup (move t0; addiu v1; li a1; sll v1; lui s0;
  addiu s0) comes BEFORE `sq ra` (instruction 14); then `sq ra` and the five FPU
  saves STRICTLY ALTERNATE with the integer min/max ops (sq ra; swc1 f24; sra
  t1; swc1 f23; subu a2; swc1 f22; addiu a3; swc1 f21; subu a1; swc1 f20; sll
  a1); the three `sll` are SPREAD across 24/26/28 interleaved with the
  occlViewParams stores; and the four fog `mov.s` are INTERLEAVED one-per-
  constant with the `lui at/mtc1` pairs (30-38).

This is the compiler's prologue-insertion + ready-list ordering, not an
instruction it cannot emit. EGC 2.95.2 groups the prologue FPU saves and the
`mov.s` fog copies; the original interleaves them. No source lever reproduces
the interleaving.

## Exhaustive attempts (all parity-safe, none closed the prologue)
- 19 source orderings (cand1..cand19): halfY/halfX order, min/max order,
  occlViewParams store order, viewCtx nearClip/farClip/xratio order, fog store
  order, xclip/yclip order, fog register-pin permutations. Some changed size
  (300/308) or broke the fog register assignment; best = cand12/cand16 (304B).
- Scheduling flags on cand16: `-fno-schedule-insns` (300B/67 diff),
  `-fno-schedule-insns2` (304B/39 diff), both (312B/76 diff). Default scheduling
  is closest (35 diff).
- `expert` (GPT-6 Astra) consulted: recommended the `"f"` barrier (APPLIED —
  fixed the tail) plus the flag matrix (tested, worse) and a scoped fog-pin
  transfer.
- `last-resort-decompiler` (GPT-5.6 Sol) invoked 2026-09-28 with the full
  dossier. Recommended two experiments:
  1. Pin the viewCtx pointer to `$s0`: `register ViewCtx* ctx asm("$16") =
     &viewCtx;` + `ctx->field`. RESULT: byte-identical to cand16 (no effect —
     s0 was already the base register).
  2. Anchor the four fog transfers at their logical positions via zero-byte
     tied `"=f"` asms with the viewCtx stores interleaved. RESULT: changed the
     size to 300 bytes / 72 diff words (worse; the prologue save structure
     changed).
- Scoped fog-pin transfer (one fog value to a normal local via `"=f"`): 308B,
  broke allocation.

## Conclusion
EGC 2.95.2 emits the correct 304-byte instruction multiset and an exact tail
(cand16), but its default scheduler chooses a different ready-list order for the
prologue (instructions 8-45): it groups the FPU saves and `mov.s` fog copies
where the original interleaves them, and delays `sq ra` to a different point.
Scheduler flags, 19 source orderings, a `$s0` pointer pin, scoped and anchored
FPR transfers were all mechanically tested and do not reproduce the original
prologue. Retain `INCLUDE_ASM(..., func_001F33B8)`; full-ELF parity preserved.

Candidate sources and linked diffs are in `working/draw_post_post_func_001F33B8/`
(cand16.cpp = best; probe16/candidate.json = authoritative 35-word linked diff).
Note: the farClip binary literal is 524288.0f, not `VIEWCTX_FAR_CLIP`; if this is
ever re-attempted it needs a named constant for that value.
