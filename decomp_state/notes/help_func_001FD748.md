# func_001FD748 — DrawGalacticMap (BLOCKED 2026-10-07)

`void DrawGalacticMap(int x0, int x1, int y0, int y1)` at VRAM 0x1FD748, size 0x4BC
(303 instrs), code/game/help.cpp. Draws the galactic-map place layer: 20 place
entries (D_001DDE28, each {x,y,val,p3}); per visible entry draws an icon
(func_001FFC30) and, for the selected slot (mapSlotPool.selected at +0x224), a
scaled label (msg_string + FontPrintLarge x2 with shadow) plus a second icon.

## Semantic understanding (complete, verified against objdump of boot_elf)
- Args: s2=x0, s0=x1, s3=y0, s1=y1. w=(x1-x0)*0x10 (in s0), h=(y1-y0)*0x10 (in s1).
- Prologue: SetupGifPaging(0); VU1_addGSregister(0x42, **0x8000000044UL**),
  (0x47,0x4B); func_00200E08(0,0,0x200,0x1C0,0x80000000,0);
  GetIconFrame(0xE99A,0xE); func_00200258(frame, **0**,0,w,h,0,0,0x80) [8-arg,
  a1=0 — the 0xE/0xF are the GetIconFrame second args, their results feed a0];
  VU1(8,0); GetIconFrame(0xE99A,0xF); func_00200258(frame,0,0,w,h,
  drawFrameCount&0xFFF,0,0x80); VU1(8,5).
- Loop: base=mapPlaceEntries, p=base+4 (int*, i.e. +16 bytes, entry 1); count=1,
  cursor=0x10. do { i=p[0]; if(i) { mode = modeA[count]?3:(modeB[count]?2:0);
  if(mode) { y=p[1]; if(pal) y=y*0x1C0/0x1A0; if(mode==3||(mode==2&&blink)) draw
  icon (GetIconFrame 0xC); if(count==selected) { text block } } } p+=4; count++;
  cursor+=0x10; } while(count<0x14). blink = (drawFrameCount % (F(22)+F(8))) < F(22).
- Text block: p3=p[3]; valA=*(base+cursor+8); ysum=y+p3; isum=i+valA; ysum1,
  isum1; vec={valAf, p3f}; **length = func_001F9B20(vec)** (returns the hypot
  sqrt(valA^2+p3^2) AS A FLOAT in $f0 — callee epilogue `qmfc2 a0,$vf1; jr ra;
  mtc1 a0,$f0`); scale=(int)(length*1000.0f); x1p=isum+((i-isum)*(scale-0x1F40))/scale;
  y1p=ysum+((y-ysum)*(scale-0x1F40))/scale; text=msg_string(mapMsgIds[count*3]);
  width=func_001F6250(text,-1); edge=(valA>=0)?(isum-width):(isum+width);
  00200C80(x1p+1,y1p+1,isum1,ysum1,0x80000000,0); 00200C80(isum1,ysum1,
  **edge+1**,ysum1,0x80000000,0); xtext=min(edge,isum); FontPrintLarge(xtext+1,
  ysum-mapFontYoffGp+1,0x80000000,text,-1); 00200C80(x1p,y1p,isum,ysum,
  0x80F0F0F0,0); 00200C80(isum,ysum,edge,ysum,0x80F0F0F0,0); FontPrintLarge(xtext,
  ysum-mapFontYoffGp,0x80F0F0F0,text,-1); GetIconFrame 0xD; func_001FFC30(frame,
  i-0xA,y-0xA,0x14,0x14,0x80).
- mapFontYoffGp (0x15F690) is read GPREL -> needs a seeded linker alias.
  videoModePal (0x15ED80), drawFrameCount (0x15F438) self-based absolute.

## SOLVED during this investigation (durable lessons)
1. **func_001F9B20 returns a float, not void/int.** The callee writes $f0 in its
   epilogue; the caller's `mul.s $f0,$f0,$f1` uses that RETURN value (the hypot
   length), NOT p3f kept across the call. My initial "p3f kept in $f0 across the
   call / needs a `register float asm("$f0")` pin" diagnosis was a MISREAD — the
   pin happened to produce the same mul.s bytes but modeled the wrong operand.
   Correct form: `float length = func_001F9B20(vec); int scale=(int)(length*1000.0f);`
   with no register pin.
2. **Symbol+constant folding:** `int* p = mapPlaceEntries + 4` folds to
   `lui %hi(mapPlaceEntries+16)` (one instr) but the original does base then +16
   (two instrs). Fix: `int* base = mapPlaceEntries; int* p = base + 4;` and use
   `base` for the valA access `*(int*)((char*)base + (cursor + 8))`. (Grouping
   `(cursor+8)` alone does NOT stop the fold — EGC reassociates and folds the 8
   into the symbol; a distinct base variable is required.)
3. **First func_00200258 a1=0** (not 0xE). 0xE/0xF are GetIconFrame args.
4. **Imperative mode dispatch fixes bnezl→bnez:** `if (p[0]!=0) { int mode=3;
   if (modeA[count]==0) mode=modeB[count]?2:0; if (mode!=0) { int i=p[0]; ... } }`
   (i declared INSIDE if(mode!=0), mode init before the if) makes EGC emit plain
   `bnez` for the modeA test and an unconditional `li a1,3` in the delay slot.
   The nested ternary `modeA?3:(modeB?2:0)` with i declared earlier emits bnezl.

## RESIDUAL BLOCKER (224 word diffs, all compiler-internal, no semantic bugs)
Best candidate (working probe1.cpp at time of block; production keeps
INCLUDE_ASM). Prologue matches through instruction index 75 (~300 bytes); the
remaining diffs are confined to the 20-iteration loop body and the selected-slot
text block, and are ALL EGC 2.95.2 default-scheduler / register-allocation /
stack-slot tiebreaks:
- **Stack-slot coloring differs:** candidate count(0x10) cursor(0x14) p(0x18)
  i(0x1C) vs original count(0x10) i(0x14) isum1(0x18) cursor(0x1C) p(0x20).
- **i is spilled early:** right after the `i!=0` test the candidate emits
  `sw i, 0x1C(sp)` (idx 81) where the original keeps i in a register through the
  mode dispatch (it loads `count` there). The candidate has higher register
  pressure (many named locals: i,y,mode,p,base,cursor,count,w,h + text-block
  temporaries).
- **vec store order reversed:** candidate stores vec[0] then vec[1] (vec[1] in
  the jal delay slot), original stores vec[1] then vec[0] (vec[0] in the delay
  slot) — semantically identical, different schedule.
- Register allocation of the mode dispatch (a0/v1/v0 vs a0/v0/v1), the PAL
  branch, and the whole text block differ instruction-by-instruction with
  identical semantics.

## Last-resort escalation (GPT-5.6 Sol) — 2026-10-07
Invoked with the full dossier (working/.../escalation_dossier.md). Returned 4
concrete forms, all applied and mechanically diffed:
- #2 float-return func_001F9B20 (removed the $f0 pin) — CORRECT, applied, no
  size/parity regression. This is the real fix; the pin was a mask.
- #3 endpoint args — call 2 third arg is **edge+1** (set in the jal delay slot
  at 0x1FDAD8), NOT isum1; xtext=min(edge,isum). Applied. (The rec's
  `*valPtr<0` sign was a typo; assembly slt/movz gives `valA>=0`.) Using a
  distinct `int* valPtr` local REGRESSED the prologue (moved the cursor store
  out of the VU1(0x42) delay slot, 223->283), so valA is computed inline.
- #1 imperative mode dispatch — fixed bnezl→bnez, applied, no diff-count change.
- #4 typed `struct MapPlace{int x,y,val,p3;}` loop — applied in isolation; NO
  change to the diff count (224) and did not fix the stack coloring.
The decompiler's own framing: the residual is the candidate's higher register
pressure (early i-spill) and stack-slot coloring, which none of the suggested
source forms (imperative mode, typed struct, valPtr) reduces under
`-G8 -O2 -ffast-math -fno-exceptions -snas`. No further strictly-C lever found.

## Resume hints (if a new technique appears)
- The whole gap is register pressure / stack coloring in a 303-instr,
  ~24-local function. A technique that forces the original's slot order
  (count,i,isum1,cursor,p) or keeps i live in a register past the i!=0 test
  would be the unlock. The `register ... asm("$fNN")` pin works for FP (camera
  Camera_Pos2Polar3d uses $f21) but no GPR pin tried here changed the slots.
- Do NOT reintroduce the `$f0` pin for func_001F9B20; the float return is the
  correct model and already matches the mul.s.
- Data tables D_001DDE28 (20x{x,y,val,p3}), D_001DDD44 (20 msg-id triples),
  D_0013DD40/D_0013DD58 (mode A/B bytes) are all zero in the boot ELF (runtime
  filled by overlays). mapSlotPool.selected is at +0x224 (needs a `s32 selected`
  added to the shared MapSlotPool struct in code/game/map.cpp if this ever matches).
