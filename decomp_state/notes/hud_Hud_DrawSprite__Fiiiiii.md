# Hud_DrawSprite__Fiiiiii (0x1FFC30, 0x1E8 = 488 bytes) — BLOCKED (2026-10-10)

VU1 data-ref **sprite** packet builder. Blocked on EGC 2.95.2
default-scheduler / register-allocation tie-breaks in the pos1/pos2/epilogue
block after the prologue + header + qwords q0–q5 were SOLVED byte-for-byte and
the pos-value SEMANTICS were corrected (z in the HIGH word). Best candidate
(`working/hud_Hud_DrawSprite/cand_v12.cpp`, production default flags) =
**488 bytes (exact size) / 40 word-diffs**; everything from the prologue through
q5 (0x1FFC30–0x1FFD44) matches byte-for-byte. The 40 residuals are ALL in
0x1FFD48–0x1FFDE4 and are pure instruction-scheduler tie-breaks.

## What it does

`void Hud_DrawSprite(int frame, int x, int y, int w, int h, int alpha)`.
Computes `HudFrame* fr = (HudFrame*)(frame*sizeof(HudFrame) + hudHeap.frames)`,
`HudFrameTex* tex = hudHeap.texs + fr->hTex`, `texU = 1<<tex->uLog`,
`texV = 1<<tex->vLog`. Appends a 16-byte header `[0x10000005, 0, 0, 0x50000005]`
plus 10 qwords at head+16 to the `vu1ChainHead` VU1 chain, then advances the
head by 20 qwords (0x50):

- q0 = 0x7400000000008001  (materialized `li t7,0xE800; dsll32 t7,t7,15; ori t7,t7,0x8001`)
- q1 = 0x5353106
- q2 = GetFrameTex(frame)   (jal 0x1FFA10)
- q3 = 0x156
- q4 = ((u64)alpha<<24) | 0x7F7F7F
- q5 = 0
- q6 = pos1
- q7 = (texV<<20) + (texU<<4)     (int `+`, not `|`, not a (u64) cast)
- q8 = pos2
- q9 = 0

where **posN is a packed (x, y, z) coordinate** (the key semantic):

```
posN = (z << 32) | (yN << 16) | xN
xN   = (xN << 4) + occlViewParams.minX - 8
yN   = (yN << 4) + occlViewParams.minY - 8
(z<<32):  z = hudHeap.vuField_0C
pos1: (x1,y1) = (x, y);   pos2: (x2,y2) = (x+w, y+h)
```

The original emits `lw a0,12(s6); dsll32 a0,a0,0; or v0,v0,a0` — i.e. it shifts
`vuField_0C` **LEFT BY 32** into the HIGH word before OR-ing it in. An earlier
candidate used `(u64)z` (low word) which was WRONG and made the function 2 words
short (the two `dsll32` were missing). Fixing it to `((u64)hudHeap.vuField_0C << 32)`
made the size match (488) and dropped the diff count 52 -> 40.

`occlViewParams.minX`/`minY` are re-loaded once per corner (4 loads total: pos1
minY@0x1FFD54, minX@0x1FFD5C, pos2 minY@0x1FFD9C, minX@0x1FFDA4) — the original
does NOT CSE them, so in C they must be `volatile s32` (they are plain `s32` in
`code/include/camera.h:221` today; see Re-attempt). `hudHeap.vuField_0C` is read
with `lw` (not `lwu`) and then `dsll32`, so it is a plain (signed) `int`, not u32.

All six `vu1ChainHead` loads are self-based absolute (`lui r,0x16; lw r,0F00(r)`);
the header/qword stores are absolute via `$at`; the final head advance
(`vu1ChainHead += 20`) is a GPREL store. Required head form in this TU: plain
(non-.data) double-volatile `extern volatile u32* volatile vu1ChainHead;`
(the `.data` form breaks the schedule).

## Register map (original, verified)

s0=alpha, s1=x, s2=p(head+0x10), s3=y, s4=texV, s5=texU, s6=&hudHeap, s7=h,
s8=w; `frame` stays in a0 through the `jal GetFrameTex`; ra saved; frame 0xA0;
prologue saves s0–s8 + ra (10 `sq`) in an interleaved order.

## The 40-word residual (all scheduler tie-breaks, 0x1FFD48–0x1FFDE4)

My EGC 2.95.2 default scheduler is more aggressive than the original build's:
1. **texflags first**: candidate computes `sll s5,texU,4; sll s4,texV,20; addu
   s4,s4,s5` FIRST (0x1FFD48–58); the original interleaves them with the pos
   shifts (`sll y,4; sll x,4; sll texU,4`) and the minX/minY loads.
2. **q7 stored early**: candidate `sd s4,56(s2)` at 0x1FFD64 (right after
   texflags); original stores it LATE at 0x1FFD88 (right after pos1).
3. **pos2 hoisted**: candidate hoists `addu s3,s3,s7` (y+h) into the pos1 block
   (0x1FFD80); original keeps it in the pos2 block (0x1FFD90).
4. **x before y**: candidate orders x-ops before y-ops; original keeps y slightly
   ahead (`y+minY`, `y<<16` before `x-8`).
5. **epilogue interleaved**: candidate interleaves `lq ra`, the head reload
   (`lui v0,0x16; lw v0,3840(v0)`), and the 9 `lq` s-register restores with the
   pos2 computation; original keeps a tight sequential epilogue after the q8 store.

## What was locked in (works)

- Plain double-volatile `vu1ChainHead` (self-based loads) in this TU.
- q0 = 0x7400000000008001 (16 hex digits; the 12-digit form is wrong).
- texflags as int `+` (not `|`, not (u64) cast) -> `addu`, and s4=texV / s5=texU.
- q[4] before q[3] source order -> store order q4,q3,q5.
- `((u64)vuField_0C << 32)` (z in high word) — the size/`dsll32` fix.
- `register struct OcclViewParams* vp asm("$5"); vp=&occlViewParams;` (base pin
  to a1) — REQUIRED; fixes an a0/a1 RA swap so 0x156 lands in a0 and the base in
  a1. A scalar pin (`register int c156 asm("$4")`) is IGNORED by EGC here.

## Escalations

- **expert (GPT-6 Astra, one-shot, 2026-10-10)**: recommended tied
  `asm volatile` dependency barriers between the corners (tie x/y/w/h as outputs
  after the pos1 store to block the pos2 hoist; tie texflags to delay the q7
  store). Tested: `"+r"(x),"+r"(y),"+r"(w),"+r"(h)` -> 112 diffs (prologue
  shattered); `"+r"(w),"+r"(h)` -> 104 diffs (still shattered). The tied
  s-register params are NOT viable — the prologue (9 interleaved s-register
  saves) is extremely allocation-sensitive.
- **last-resort-decompiler (GPT-5.6 Sol, 2026-10-10)**: recommended "phase
  fences" using only post-call temporaries (input-only `asm("" : : "r"(pos1),
  "r"(texFlags) : "memory")` before the q7/q6 stores; scoped late transfers
  `asm("" : "=r"(pos2W), "=r"(pos2H) : "0"(w), "1"(h) : "memory")` to block the
  pos2 hoist; a final `asm("" : : : "memory")` to hold the epilogue). Tested all
  three incrementally: producer-fence-only = 488B/43 (10 prologue diffs); full =
  492B/60 (17 prologue diffs, +4 bytes); separate-transfer fallback = 488B/55
  (17 prologue diffs). Every fence perturbs the prologue and is WORSE than v12.

## Verdict

BLOCKED. The pos semantics are fully solved (size matches, prologue + q0–q5
byte-correct). The 40 residuals are coupled EGC 2.95.2 default-scheduler / RA
tie-breaks in the pos1/pos2/epilogue block. Every lever that constrains the
scheduler (tied barriers, phase fences, register pins on the params, both
scheduler flags, source reordering) perturbs the allocation-sensitive prologue
and makes the match WORSE. This matches the sibling VU1-packet-builder blocker
class (`framebuf_func_001FB8F0`, `draw_post_DoGifPaging__Fv`). Retain
INCLUDE_ASM; full-ELF parity preserved (no source change committed).

## Re-attempt start point

`working/hud_Hud_DrawSprite/cand_v12.cpp` (base pin + `(u64)z<<32`). If revisited:
(1) make `OcclViewParams.minX`/`minY` `volatile s32` in camera.h (the original
re-loads per corner; currently plain s32) and `hudHeap.vuField_0C` a signed int
in hud.h — both were verified to be the original's form but are REVERTED here
since the function stays INCLUDE_ASM; (2) the residual is the pos/epilogue
scheduler, so any attempt needs a scheduler constraint that does NOT touch the
9 s-register params.
