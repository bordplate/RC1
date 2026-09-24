# camera_func_001ED7F0 — Camera_checkCollLine (BLOCKED)

Address 0x1ED7F0, size 0x14C (332 bytes / 83 words). `code/game/camera.cpp` line 664.
Symbol `Camera_checkCollLine__Fv` (0 args). Per-frame camera line-collision check.
Retained as `INCLUDE_ASM`; full boot ELF parity preserved.

## Semantics (objdump-verified)
The function casts a vertical line segment through the camera position against the
level collision, stepping it back until it is clear or a 6-iteration budget runs out.

```c
typedef unsigned int CamQuad __attribute__((mode(TI)));   // 128-bit -> lq/sq
union CamPt { CamQuad q; struct { float x,y,z,w; } v; };  // 16 bytes
// currentCamera @0x186F40: pos Vec4 {x,y,z,w} @0x140..0x14F; pCur u32 @0x180
//   -> UpdateCam { s16 collMode @0x86 }; collValue u32 @0x394.
// camCollStaged: s32 @0x15F624 (gp window, GPREL16).
// CollOutput @0x194100 (outside gp window -> absolute): { pad[8]; float point[4] } -> point @0x194120.

void Camera_checkCollLine(void) {
    CamPt a, b;
    UpdateCam* pCur = (UpdateCam*)currentCamera.pCur;
    if (pCur->collMode == 6 || camCollStaged != 0) { currentCamera.collValue = 0; return; }
    volatile CamQuad* pQ = (volatile CamQuad*)&currentCamera.posX;  // NOT CSE'd: lq twice
    a.q = *pQ;  b.q = *pQ;          // 128-bit: a,b = {x,y,z,w} = cam.pos (a.z = pos.z, NOT uninitialized)
    a.v.z += 0.75f;  b.v.z -= 0.75f;  // vertical segment pos.z +/- 0.75
    int count = 0;
    while (count < 6 && CollLine_Fix(&a, &b, 18, 0, 0) != 0) {      // CollLine_Fix=0x1efa68 C linkage, 5th arg t0
        if (func_001F0B58() == 0) {                                  // 0x1f0b58 C placeholder
            float r3 = func_002135F0(CollOutput.point, 0);           // 0x2135f0, returns float
            if (currentCamera.posZ < r3 + 0.04f) currentCamera.collValue = 1;  // stores CONST 1
            else currentCamera.collValue = 0;
            return;
        }
        a.q = *(CamQuad*)CollOutput.point;   // 128-bit copy of the collision point
        a.v.z -= 0.01f;
        count++;
    }
}
```

Key verified facts:
- The a/b setup and the CONT update are **128-bit `lq`/`sq`** (16-byte) copies, so a.z/b.z
  are initialized to pos.z / the point's z — NOT uninitialized stack reads. (A 64-bit `long xy`
  model is wrong; it emits ld/sd and gives misleading RA.)
- The true-path store stores the constant **1** (the `li v0,1` in the `bc1t` delay slot clobbers
  the float return before `sw v0,916(v1)`). `(int)r3` is wrong (emits cvt.w.s/mfc1).
- The two initial 128-bit loads from cam.pos are NOT CSE'd (`lq` twice).
- Loop header is `slti count,6; beqz EXIT; [setup]; jal CollLine; bnez BODY; fall-through EXIT`
  == combined condition `while (count<6 && CollLine()!=0)`.
- FP constants 0.75 (f2) / 0.04 (f1) / 0.01 (f0) are materialized in temporaries IN-PATH.
- Original callee-saved set: **s0=count, s1=&b (sp+16), s2=%hi(currentCamera)=0x18. No callee-
  saved FPRs. No callee-saved for the CollOutput base.** The CollOutput base (0x190000) is
  re-materialized per path: `lui v1,0x19` in the F0B58-`bnez` delay slot (CONT) and
  `lui a0,0x19` in FAIL (then `addiu a0,a0,16672` in the 0x2135f0-`jal` delay slot = &point).
  &b is set up ONCE in MAIN (`addiu a0,sp,16`) then `move s1,a0` in the `b TOP` delay slot;
  the loop uses `move a1,s1`.

Constants (python-struct confirmed): 0x3F400000=0.75f, 0x3C23D70A=0.01f, 0x3D23D70A=0.04f.

## Best candidate: v16 — 79 words, 59 byte diffs (floor)
v16 = the corrected C above, PLUS:
- combined-condition loop (matches loop topology),
- FP register pins: FAIL `register float tolerance asm("$f1")=0.04f; asm volatile("" : "+f"(tolerance));`
  and CONT `register float retreat asm("$f0")=0.01f; asm volatile("" : "+f"(retreat));`
  — these DEFEATED the f20/f21 callee-saved FP hoisting + FPR spills (frame 0x60 matches),
- tied GPR barriers before each CollOutput.point use: `float* pf=CollOutput.point; asm volatile("" : "+r"(pf));`
  (FAIL) and `CamQuad* pc=(CamQuad*)CollOutput.point; asm volatile("" : "+r"(pc));` (CONT).

v16 matches: prologue, gate, 128-bit lq/sq copies, loop topology (TOP/P2CALL/FAIL/CONT/EXIT),
the `li v0,1; sw v0,916(v1)` true-path store, in-path FP constant materialization (f1/f0),
frame 0x60, no FPR spills.

### Residual (59 diffs; v16 is 79 words, original 83 — 4 words short)
1. **s1 ownership (the core):** v16 hoists the loop-invariant CollOutput base into **s1**
   (`lui s1,0x19` in MAIN, then `addiu v0/a0,s1,32` per path for &point). Original keeps
   **s1=&b** and re-materializes the base per path (`lui 0x19`). Consequence: original loop
   setup `move a1,s1`; v16 `addiu a1,sp,16` (recomputed each iteration).
2. MAIN ordering: original = `lq a;sq a; addiu a0,sp,16; lq b; sq b,0(a0); lwc1 a.z; [move s0,0];
   0.75; lwc1 b.z; add.s; sub.s; swc1 a.z; swc1 b.z; b TOP; [move s1,a0]`. v16 interleaves the
   hoisted `lui s1`, reorders the two lq/sq pairs (b stored to 16(sp) not 0(a0)), and puts
   `swc1 b.z` in the `b TOP` delay slot instead of `move s1,a0`.
3. `addiu v1,a1,320` (the `&cam.pos` base for the lq) sits in the `beqz v0,MAIN` delay slot in
   the original; v16 uses `a1+320` directly and leaves the delay slot a `nop`.
4. FAIL register homes: original loads posZ into f2 (`lwc1 f2,328(v1)`, `c.lt.s f2,f0`) and
   `addiu v1,s2,28480` (full lo of &cam); v16 loads posZ into f1 (`lwc1 f1,328(v1)`,
   `c.lt.s f1,f0`) and `addiu v1,s2,0`.
5. The 4-word length difference (83 vs 79) = original's per-path `lui` (one more than v16's
   single hoisted lui) + original's `&b` setup (`addiu a0,sp,16`+`move s1,a0`) that v16
   replaces with a per-iteration `addiu a1,sp,16`.

## Exhausted attempts (all measured via tools/decomp_probe.py)
- v1–v11: 64-bit `long xy` copy (WRONG width) — 77–84 diffs.
- v12: 128-bit CamQuad copies — 84, frame 0x90.
- v13: + combined-condition loop — 80 (fixed loop topology).
- v14: + FP pins `$f1`/`$f0` — 74 (removed f20/f21 hoisting + spills).
- v15: v14 + `-mno-split-addresses` — 77 (worse; folds cam addressing).
- **v16: v14 + tied GPR barriers — 59 (best), frame 0x60.**
- v17: v16 + input-only barrier `asm volatile("" : : "r"(&CollOutput))` — 81 (worse; hoists to 5 regs).
- v18: v16 + `register CamPt* pb asm("$16")=&b` — 74 (WRONG: $16 is s0; pinned &b to s0, count to s1).
- v19: v16 + `asm("$17")` (=s1) pin — 74 (pin to a callee-saved reg backfires).
- v20: v16 + scoped transfer (`register volatile CamQuad* pQ asm("$3")=...; register CamPt* initialB asm("$4")=&b; initialB->q=*pQ; ...; asm volatile("" : "=r"(loopB) : "0"(initialB));`) — 66.
  Landed `addiu v1,a1,320` in the beqz delay slot + `addiu a0,sp,16`, but &b->s2 (not s1),
  hi(cam)->s3 (not s2), and s1 STILL the hoisted base; frame grew to 0x70 (4th saved reg).
- v24: v20 but b.z via `b` local (sp-based) — 69.
- v25: v16 + scoped `$4` transfer only (no `$3` pQ pin) — 67.
- v21: v16 + distinct same-address aliases (collOutputFail/collOutputContinue @0x194100), NO barriers — 73.
- v22: v20 + aliases, NO barriers — 82.
- v23: v21 + path-local tied barriers — 74.
- Flags on v16 (none changed the 59): `-fno-rerun-loop-opt`, `-fno-strength-reduce`,
  `-fno-rerun-loop-opt -fno-strength-reduce`. Earlier (v13/v14): `-fno-schedule-insns`(83),
  `-fno-schedule-insns2`(84), `-fno-move-all-movables`(81), `-fno-gcse`(81),
  `-fno-gcse -fno-schedule-insns2`(80), `-fno-rerun-cse-after-loop`(77),
  `-fno-expensive-optimizations`(80), `-fno-cse-follow-jumps -fno-cse-skip-blocks`(88),
  `-mno-split-addresses`(77). `-fno-cse` is not a valid option in this cc1plus;
  `-ffixed-$f20/-ffixed-$f21` are no-ops (bad syntax).

## Blocker characterization
A coupled register-allocation/scheduling difference, not a source-form problem:
- Local EGC merges the two path-specific `CollOutput.point` addresses into one loop-invariant
  high-page value and reload-promotes it into a callee-saved GPR (s1).
- That displaces the call-live `&b` pointer, so the loop recomputes `addiu a1,sp,16` each
  iteration instead of `move a1,s1`, and the base is a single hoisted `lui` instead of the
  original's two per-path `lui 0x19`.
- The secondary MAIN ordering, posZ f1-vs-f2, and &cam `addiu` differences follow from that
  changed live-range graph. The result is a 79-word function vs the original's 83 words.
No C formulation (24 variants: pins, scoped transfers, barriers, aliases, width/loop/FP forms)
and no cc1plus flag tested flips the local EGC's choice to keep the CollOutput base in a
callee-saved GPR rather than `&b`. This matches the sibling camera-collision blockers
(func_001ED470, func_001EC8A0, func_001ECCD8) — EGC 2.95.2 scheduler/RA on the camera
128-bit-copy class.

## Escalations
- expert (GPT-6 Astra) one-shot (prior session): the 128-bit `lq`/`sq` correction (the key
  unlock — a.z/b.z are pos.z, not uninitialized), the 1/0 flag store, the combined-condition
  loop, the FP pin register homes, and the GPR base-barrier idea. All applied.
- last-resort-decompiler (GPT-5.6 Sol) invoked 2026-09-25 on this exact target: (1) corrected
  that `$16`=s0 not s1 (my v18 pinned the wrong reg); (2) recommended the scoped `$4`->local
  transfer + `$3` pQ pin (v20/v25 — 66/67, &b->s2 not s1, base still hoisted); (3) distinct
  same-address linker aliases for the two CollOutput uses (v21-v23 — 73-82, worse); (4)
  `-fno-rerun-loop-opt`/`-fno-strength-reduce` (no change). All mechanically tested; none
  produced a match or beat v16 (59).

## Probe command
Best candidate (v16) preserved at `decomp_state/notes/camera_func_001ED7F0.candidate.cpp`.
```sh
source .venv/bin/activate
python3 tools/decomp_probe.py decomp_state/notes/camera_func_001ED7F0.candidate.cpp \
  code/_generated/nonmatchings/game/camera/func_001ED7F0.s Camera_checkCollLine__Fv \
  --define camCollStaged=0x15f624 --define func_001F0B58=0x1f0b58 --define func_002135F0=0x2135f0 \
  --out /tmp/opencode/camcoll-v16
```
