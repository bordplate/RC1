# actuator_CalcPower (0x1E8D08, size 0x414) — BLOCKED

Status: BLOCKED (2026-09-21). expert (GPT-6 Astra) + last-resort-decompiler (GPT-5.6 Sol)
both invoked and their recommendations applied. Two INDEPENDENT hard blockers remain:
(A) saved-register + stack-frame allocation, (B) switch-table section placement.
Kept as INCLUDE_ASM to preserve boot-ELF parity. See "Blockers (2026-09-21)" and
"Corrected candidate (start point)" below.

## What it is
Per-frame shock/actuator mixer. `power` (param) is a 2-int array (left/right) that the
caller reads as the final actuator output. It maintains two local 2-int accumulators
`scale[2]` and `numscale[2]` and a 2-int `numpower[2]` count, and mixes up to 8
`ActuatorWave` control points into them. Returns the number of active (type!=0) waves.

## Symbol
- `actuator_CalcPower = 0x001e8d08` (config/symbols.txt:290). UNMANGLED (no cfront
  suffix, unlike Deadlocked's `actuator_CalcPower__FPiP3PAD`). C++ def needs an asm
  override: `int actuator_CalcPower(int* power) asm("actuator_CalcPower");`
  (precedent: code/game/lights.cpp:14). Boot ELF is stripped; name chosen from DL.

## Struct (see code/include/actuator.h)
`actuatorWave` is 16 bytes, `ActuatorWave[8]` at 0x00165480:
```
+0x00 s16 type      (lh)  wave type 0-5
+0x02 u8  side      (lbu) 0=left 1=right
+0x03 u8  scale     (lbu) non-zero selects the scale accumulator
+0x04 s16 delay     (lh)  frames before the wave starts
+0x06 u16 lifeSpan  (lhu) frames remaining; type cleared when it reaches 0
+0x08 s16 timer     (lh/lhu) cycling position
+0x0A s16 on        (lh)  on-phase length
+0x0C s16 off       (lh)  off-phase length
+0x0E u8  power     (lbu) peak power
+0x0F u8  minpower  (lbu) floor power
```

## Frame / saved registers (ORIGINAL)
`addiu sp,sp,-0xC0`; saves s0@0x30 s1@0x40 s2@0x50 s3@0x60 s4@0x70 s5@0x80
s6@0x90 ra@0xA0; `swc1 f20,0xB0(sp)`.
- s0=$16 = &ActuatorWave[i]  (aw)
- s1=$17 = loop counter (i)   — reused by all 3 loops
- s2=$18 = power (param)
- s3=$19 = numpower base (sp+0x20)
- s4=$20 = numscale base (sp+0x10)  (never written in wave loop; dead array except post-switch/final)
- s5=$21 = ret
- s6=$22 = hi(ActuatorWave) = 0x16 (lo 0x5480)
- clear-loop temps: v1=sp(scale), a0=numscale, v0=power, a1=numpower
- final-loop temps: a0=power(s2), a2=sp(scale), a3=numpower(s3), t0=numscale(s4)

## Control flow
1. Clear loop (i=1..0): zero scale[2], numscale[2], power[2], numpower[2] (one int
   each pass; 4 stores/pass; `bgez s1`).
2. Wave loop `for (i=0;i<8;i++)`: aw=&ActuatorWave[i]; if type==0 skip; else:
   - ret++
   - lifeSpan: `lhu life; sh life-1; if (life==0) type=0` (signed test via sll16+bgtz)
   - if delay!=0: delay--; continue
   - uVar12=0; timer++; pos = timer % (on+off)
   - switch(type):
     - 0: break
     - 1: if pos<on: uVar12 = power+minpower
     - 2: if pos<on: uVar12=(power*pos)/on else (power*((off+on)-pos))/off; uVar12+=minpower
     - 3: if pos<on: uVar12=(power*pos)/on+minpower
     - 4: if pos>=on: uVar12=(power*((off+on)-pos))/off+minpower
     - 5: (degenerate FP, see below)
   - post-switch: if scale!=0: scaleArr[side]+=uVar12 (+ DEAD `addu a0,s4,a0`=&numscale[side]);
     else power[side]+=uVar12 (side reused from beqzl(1e8fe8) delay-slot load 1e8fec);
     then ALWAYS numpower[side]++ (fresh lbu a0,2(s0)).
3. Final loop (i=1..0):
   ```
   if (numpower[i] != 0) power[i] /= numpower[i];
   pPower++;
   if (numscale[i] != 0) { q = scale[i]/numscale[i]; scale[i]=q; power[i]=power[i]*q/255; pPower++; }
   scale++; numscale++; numpower++;
   ```
   NOTE the `power` (a0) pointer advances +1 always (1e909c delay slot) and +1 more in
   the scale branch (1e90d4); the scale branch reads/writes power[i+1] (already-advanced a0).
   `beql <div>; break` = EGC div-by-zero guard.
4. return ret.

## Case 5 (0x1E8F34) — DEGENERATE FP (reproduce literally)
pi = 0x40490FDB = 3.1415927f. 0x3F000000 = 0.5f. conv = func_001FA6C0 (int->float).
- THEN (pos<off): `conv(pos)` [DEAD], `conv(off)`->f20/f0, f1=pi, `f20*=pi`, `b`,
  `f20 = f20/f0`  (=> pi)
- ELSE (pos>=off): `conv(pos-off)` [DEAD], `conv(on)`->f20/f0, f1=pi, `f20*=pi`,
  `f20 = f20/f0`, `f20 = pi - f20`  (=> 0)
- `FastCos(f20)` result DEAD (clobbered before use; call kept).
- tail: pf=conv(power) [one call, f0/f20], `f1 = pf*pf + pf`, [DEAD] `f12 = (pf^2+pf)*0.5`,
  `conv2 = func_001FA6D0(pf^2+pf)`, `t0 = conv2 + minpower`, `if (t0>255) t0=255`.
  (The `*0.5` (f12) is computed but unused.)

## C candidate (logically correct; does NOT byte-match)
Saved in git history / this note. Key shape: asm override; pointer idiom for clear +
final loops; `for(i=0;i<8;i++)` wave loop; lifeSpan decrement fix; delay/timer-modulo;
switch 0-5; post-switch; final loop with mid-body `pPower` double-advance.

## Remaining gaps (why it does not byte-match yet)
1. SAVED-REGISTER PERMUTATION: original numpower->s3, numscale->s4, ret->s5, AWhi->s6;
   my EGC emits ret->s3, numpower->s4, AWhi->s5, numscale->s6. Driven by EGC liveness /
   first-use order, not declaration order (reordering decls did NOT change it).
   The 4 permuted vars are stack/hi-base values stored at sp+0x60/0x70/0x80/0x90, so the
   permutation touches every use of numpower/numscale/ret/AWhi -> many word diffs.
8. SWITCH-TABLE SECTION (confirmed 2026-09-21, compiler probe): the original switch
   dispatch (0x1E8E10-0x1E8E3C) references `jtbl_001E7640`, a 6-word table in the
   .data blob (file 0xE85C0, offset 0x821C0 into .data; followed by 2 pad words then
   string data). EGC 2.95.2 ALWAYS emits a C `switch` table into its own `.rdata`
   section (probe-verified with the exact project flags: `-G8 -O2 -ffast-math
   -fno-exceptions -snas`); Splat's linker script places every .o(.rodata) in the
   `.text` output section AFTER all .data, so EGC's table lands in .text at a local
   `$Lxx` label, NOT in .data at 0x1E7640. The dispatch instruction pattern is
   byte-identical (`lui %hi; sll 2; addiu %lo; addu; lw; jr`) — ONLY the two
   %hi/%lo table-address words (0x1E8E24/0x1E8E2C) differ. No C source form makes
   EGC emit the table in .data; the only fix is fragile linker-script surgery
   (split the .data blob around 0x1E7640 + inject actuator.o(.rodata) into the .data
   output section at that offset), which the auto-generated SCUS_971.99.ld does not
   support without a post-split patch hook.
2. DEAD `addu a0,s4,a0` (1e9004) = &numscale[side] in the scale!=0 post-switch path.
3. DEAD `addiu a2,s6,0x5480` (1e9050) = &ActuatorWave hoisted to the wave-loop exit
   (immediately overwritten by `move a2,sp` at 1e9068).
4. Case-5 degenerate FP: dead first conv call, dead (pf^2+pf)*0.5, exact intermediate
   register structure (f20 vs f12), and FP-reg-across-call scheduling
   (`mul.s f12,f1,f12` uses f1 after the conv2 call).
5. lifeSpan signed test: original `lhu;addiu -1;sll 16;bgtz;sh(delay);sh zero`;
   candidate emitted `bnez`.
6. timer modulo: original plain `lh` dividend; candidate emitted redundant
   `sll 16;sra 16`.
7. Final-loop FP/int scheduling and pointer-register assignment differ.

## Session 2026-09-21 (blocked) — corrections + blockers

### Reconstruction corrections (from expert + last-resort; verified against objdump)
The prior candidate had MAJOR logic errors. Corrected forms:
- **Count increment is NOT always `numpower[side]++`.** The post-switch (0x1E8FE4-0x1E9040)
  selects a `count` pointer then `*count++`:
    ```c
    int *count;
    if (aw->scale == 0) { power[aw->side] += pow; count = numpower + aw->side; }
    else                { count = numscale + aw->side; scale[aw->side] += pow; }
    (*count)++;
    ```
  So `numscale` IS used in the wave loop (scale path) — its base has long liveness, which
  is why the original parks it in a high-priority saved reg. (The earlier "dead addu
  a0,s4,a0" and "dead" notes were this count-pointer selection, not dead code.)
- **pos = `timer / (on+off)`** — objdump 0x1E8E04 is `div zero,v0,v1` (dividend=timer)
  and 0x1E8E18 is `mfhi` = QUOTIENT. It is integer DIVISION, not the modulo the prior
  note assumed.
- **Case 1**: `pow = (pos<on) ? (power+minpower) : minpower;` (pos>=on gives minpower,
  not 0 — the minpower load is in the beq delay slot at 0x1E8E4C).
- **Case 5 is REAL FP, not degenerate** (prior misread of delay slots):
    ```c
    float f;
    if (pos < aw->off)      f = ((float)pos * PI) / (float)aw->off;
    else                    f = PI - ((float)(pos-aw->off) * PI) / (float)aw->on;
    f = FastCos(f);
    pow = (int)((f * (float)aw->power + (float)aw->power) * 0.5f) + aw->minpower;
    if (pow > 255) pow = 255;
    ```
  The "dead" conv/FastCos calls were delay-slot value preservation (f20 holds the first
  conv across the second conv; f20 holds FastCos across the power conv). PI=0x40490FDB.
- **lifeSpan**: `u16 life=aw->lifeSpan; aw->lifeSpan=life-1; if ((s16)life<=0) aw->type=0;`
  matches `lhu; addiu -1; sll old,16; bgtz; sh new; sh 0`.

### Blocker A: saved-register + stack-frame allocation (the gate)
Original: s0=aw, s1=i, s2=power, s3=numpower(sp+0x20), s4=numscale(sp+0x10), s5=ret,
s6=AWhi(0x16); frame 0xC0.
- Natural corrected source (no hacks) compiles to s3=ret, s4=numpower, s5=numscale,
  s6=AWhi (s6 matches; s3/s4/s5 cyclically permuted) and frame 0xB0.
- `-fno-schedule-insns` does NOT change the allocation (probe-verified: identical
  value->register map, only instruction order changes). Declaration reordering (prior
  session) also had no effect. Allocation is RTL/live-range-driven.
- Forcing `register int*pNumpower asm("$19"); register int*pNumscale asm("$20");
  register int ret asm("$21");` DOES produce s3=numpower,s4=numscale,s5=ret,s6=AWhi,
  BUT it is an unreliable non-clean hack and still leaves the stack FRAME at 0xB0
  (orig 0xC0) and swaps the numpower/numscale STACK slots (s3/s4 hold sp+16/sp+32
  opposite to the original). Full objdump diff of the asm()-forced candidate: 243/261
  words differ. Not a clean or working match.
- Verdict: matching the allocation requires the exact original source's RTL (local
  declaration order, pointer-vs-array shapes, and the 0xC0 frame layout). Not reachable
  with flags or clean source restructuring.

### Blocker B: switch-table section placement
Original switch dispatch (0x1E8E10-0x1E8E3C) references `jtbl_001E7640`, a 6-word table
in the .data blob (vram 0x1E7640, file 0xE85C0). The table entries are ABSOLUTE words
(0x1E8FE4/0x1E8E40/0x1E8E60/0x1E8EC0/0x1E8EF4/0x1E8F34) — NOT relocatable. EGC 2.95.2
always emits a C `switch` table into its own `.rdata` (probe-verified with exact flags),
which the linker places in `.text`. So:
  - An ordinary `switch` -> table in .text, two %hi/%lo dispatch words differ.
  - An external computed-goto (`goto *jtbl_001E7640[type]`) CANNOT work: the table holds
    absolute original case-body addresses, so it would jump to the wrong place unless the
    recompiled case bodies land at the exact original addresses (circular).
  - Only fragile linker-script surgery (split the .data blob around 0x1E7640 + inject
    actuator.o(.rodata) into the .data output section) could place EGC's relocatable table
    there; the auto-generated SCUS_971.99.ld has no hook for that.

### Bottom line
Both blockers are independent and hard. A future attempt should start from the corrected
candidate below, get the exact original local-declaration layout (for the 0xC0 frame +
s3/s4 stack slots), and needs a linker-script patch hook for the switch table.

## Corrected candidate (start point for a future attempt)
```c
int actuator_CalcPower(int *power) asm("actuator_CalcPower");
int actuator_CalcPower(int *power) {
    int scale[2], numscale[2], numpower[2];  // decl order = stack order sp+0x00/0x10/0x20
    int i, ret = 0, pos, pow = 0;
    struct actuatorWave *aw;
    { int *a=scale,*b=numscale,*c=numpower,*p=power;
      for (i=1;i>=0;i--){*a++=0;*b++=0;*p++=0;*c++=0;} }
    for (i=0;i<8;i++){
        aw=&ActuatorWave[i];
        if (aw->type==0) continue;
        ret++;
        { u16 life=aw->lifeSpan; aw->lifeSpan=life-1; if ((s16)life<=0) aw->type=0; }
        if (aw->delay!=0){ aw->delay--; continue; }
        pow=0; aw->timer++; pos=aw->timer/(aw->on+aw->off);
        switch (aw->type){
        case 0: break;
        case 1: pow = (pos<aw->on) ? (aw->power+aw->minpower) : aw->minpower; break;
        case 2: pow = (pos<aw->on) ? (aw->power*pos)/aw->on
                                    : (aw->power*(aw->off+aw->on-pos))/aw->off;
                 pow += aw->minpower; break;
        case 3: if (pos<aw->on) pow=(aw->power*pos)/aw->on+aw->minpower; break;
        case 4: if (pos>=aw->on) pow=(aw->power*(aw->off+aw->on-pos))/aw->off+aw->minpower; break;
        case 5: { float f;
                  if (pos<aw->off) f=((float)pos*3.1415927f)/(float)aw->off;
                  else             f=3.1415927f-((float)(pos-aw->off)*3.1415927f)/(float)aw->on;
                  f=FastCos(f);
                  pow=(int)((f*(float)aw->power+(float)aw->power)*0.5f)+aw->minpower;
                  if (pow>255) pow=255; break; }
        }
        { int *count;
          if (aw->scale==0){ power[aw->side]+=pow; count=numpower+aw->side; }
          else             { count=numscale+aw->side; scale[aw->side]+=pow; }
          (*count)++; }
    }
    { int *a=scale,*b=numscale,*c=numpower,*p=power;
      for (i=1;i>=0;i--){
          if (*c!=0) *p=*p/*c;
          p++;
          if (*b!=0){ int q=*a/*b; *a=q; *p=*p*q/255; p++; }
          a++; b++; c++; } }
    return ret;
}
```
(Note: local array DECLARATION ORDER must place numscale at sp+0x10 and numpower at
sp+0x20 to match the original's s4=sp+16 / s3=sp+32; the natural order above does not,
and the 0xC0 frame vs 0xB0 is still unexplained — likely an extra 16-byte local in the
original. See Blocker A.)

## Diff pipeline
`tools/mips64r5900el-ps2-elf/usr/bin/mips64r5900el-ps2-elf-objdump -d --start-address=0x1e8d08
--stop-address=0x1e911c assets/boot_elf.elf` (only objdump that works on the EE binary)
vs objdump of the candidate .o.

## Session 2026-09-22 (re-examined; block re-confirmed)

Re-attack from the corrected candidate (standalone EGC compile, default flags). Blockers
A and B re-confirmed; two new details:

- **Blocker A re-confirmed mechanically.** Corrected candidate (decl order
  `scale, numscale, numpower`) compiles to: s0=aw, s1=i, s2=power, **s3=ret(zero),
  s4=sp+0x20, s5=sp+0x10**, s6=hi(ActuatorWave), **frame 0xB0, no `swc1 f20`**.
  Original: s3=numpower(sp+0x20), s4=numscale(sp+0x10), s5=ret, s6=AWhi, frame 0xC0
  with `swc1 f20,0xB0(sp)`. The 0x10 frame delta = the f20 save (8B) + an 8B gap
  (0xA8-0xAF) the candidate does not reserve; the local region is the same 48B
  (three 16-byte slots at sp+0x00/0x10/0x20) in both. RTL-driven allocation unchanged.
- **func_001FA6D0 (0x1FA6D0) is `trunc.w.s $f12,$f12; mfc1 v0,$f12`** (objdump ground
  truth; splat mislabels the word `cvt.w.s`). It is the EE TRUNC.W.S variant: it reads
  its float argument from **$f12** (not the standard $f0) and truncates toward zero.
  The case-5 caller pre-positions f12 in the `jal` delay slot
  (`jal; mul.s f12,f1,f12`), so the C form `func_001FA6D0(expr)` (arg in f0) CANNOT
  reproduce the call — the value must land in f12 via the delay slot. This is a third,
  independent codegen wall in case 5 on top of A and B.
- **Blocker B has no clean C path (refined).** The 6-word table at 0x1E7640 already
  exists in the .data blob (data.data.o bytes, match by construction) and the linker
  script already defines `jtbl_001E7640 = 0x1e7640`. A hand-written inline-asm dispatch
  through that symbol would avoid EGC's .rdata — BUT a plain C `switch` is still
  required to emit the case bodies at the original absolute addresses the table
  entries reference, and a plain switch forces EGC's .rdata table (which our linker
  script places in the text .rodata region, shifting every byte after it → parity
  break). The dispatch + case bodies therefore cannot be split between clean C and
  asm; the only forms are (a) plain switch → 2 dispatch words + .rodata shift, or
  (b) hand-write the whole switch in asm ≈ retaining INCLUDE_ASM. No linker-script
  hook exists to relocate EGC's .rdata into the .data blob.
- GNU computed-goto probe (`goto *target`) compiles to a beq/slt comparison chain +
  local table (no .data reference) — does not reproduce the original table dispatch.

Conclusion: unchanged. Retain INCLUDE_ASM. A future match would need the exact
original RTL for the s3/s4/s5 + f20 allocation (A), a table-in-.data placement
mechanism (B), and an f12 delay-slot FP convention (case 5) — three independent walls.
