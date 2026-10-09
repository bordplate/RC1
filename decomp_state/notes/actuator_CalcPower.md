# actuator_CalcPower (0x1E8D08, 0x414)

`extern "C" int actuator_CalcPower(int *power)` in `code/game/actuator.cpp` (INCLUDE_ASM at line 9).
`power` is an `int[2]` output (side 0/1). Returns the count of active waves.

## Full decode (settled)

Struct `actuatorWave` (offsets): type s16@0x0, side u8@0x2, scale u8@0x3, delay s16@0x4,
lifeSpan u16@0x6, timer s16@0x8, on s16@0xA, off s16@0xC, power u8@0xE, minpower u8@0xF.
`extern actuatorWave ActuatorWave[8]` (base 0x165480).

Helpers: `float func_001FA6C0(int)->f0`, `int func_001FA6D0(float f12)->v0`,
`float FastCos(float f12)->f0`. PI=3.1415927f (0x40490FDB), 0.5f=0x3F000000.

1. **Zero-init** of `scale[2], numscale[2], numpower[2], power[2]` via a pointer-increment
   do-while (counter from 1 down). Body order (matches original byte-for-byte in structure):
   `*pScale=0; i--; *pNumscale=0; pScale++; *pPower=0; pNumscale++; *pNumpower=0; pPower++; pNumpower++;`
2. **Main loop** over `ActuatorWave[8]` (`while(1){... i++; if(i>=8)break;}`), element
   `aw=&ActuatorWave[i]`. For `type!=0`:
   - `active++`.
   - lifeSpan: `u16 life=aw->lifeSpan; aw->lifeSpan=life-1; if((s16)life<=0) aw->type=0;`
     (load old ONCE; store old-1 in the bgtz delay slot [always]; clear type only if (s16)old<=0).
   - if `delay!=0`: `aw->delay=aw->delay-1; goto loop_inc;`
   - else: `pow=0; aw->timer++; q = aw->timer % (aw->on+aw->off);` then `switch(type)`
     (jump table `jtbl_001E7640`):
     - case 0: (none)
     - case 1: `if(q<on) pow=power+minpower; else pow=minpower;`
     - case 2: `if(q<on) pow=(power*q)/on; else pow=(power*(off+on-q))/off; pow+=minpower;`
     - case 3: `if(q<on) pow=(power*q)/on+minpower;`
     - case 4: `if(q>=on) pow=(power*(off+on-q))/off+minpower;`
     - case 5 (FPU):
       `if(q<off) angle=fC0(q)*PI/fC0(off); else angle=PI-fC0(q-off)*PI/fC0(on);`
       `angle=FastCos(angle); pf=fC0(power); pow=fD0((pf*angle+pf)*0.5f)+minpower; if(pow>0xFF)pow=0xFF;`
       The FastCos result is preserved into f20 in the `jal fC0(power)` DELAY SLOT
       (`mov.s f20,f0`), so the math is genuinely `(cos*pf + pf)*0.5` — NOT an f0-clobber.
   - accumulate: `if(scale==0){power[side]+=pow; pCount=&numpower[side];} else {pCount=&numscale[side]; scale[side]+=pow;} *pCount+=1;`
3. **Post-loop** normalize (pointer-increment do-while, counter from 1 down):
   `if(*pNumpower) *pPower/=*pNumpower; if(*pNumscale){ pPower++; *pScale/=*pNumscale;
   *pPower=(*pPower**pScale)/0xFF; pPower++; } pScale++; pNumscale++; pNumpower++; i--;`
   (power pointer advances +4 always via the `beql` delay slot, and +4 more inside the
   numscale!=0 branch = +8 there, +4 otherwise).

**EGC dead-mult**: every `a*b/c` emits `mult a,b; div a,c; mflo` (uses original `a`,
discards the product). Reproduce by writing the literal `a*b/c` expressions; do not "fix".

## Best candidate

`working/actuator_CalcPower/candidate.cpp` — semantically correct after the last-resort
corrections (lifeSpan load-once; case 1 `else pow=minpower`; case 5 `+ pf` not `+ angle`).
Probe: 256 word diffs, candidate 0x400 vs original 0x414 (5 words short). First diff at
0x1E8D0C (the frame-decrement word 0x1E8D08 already matches — `subu sp,sp,192` assembles
to the same `addiu sp,sp,-0xC0` word).

## Blockers (independent, both confirmed by last-resort GPT-5.6 Sol)

1. **Reload register-coloring permutation.** Original: numpower base->s3, numscale base->s4,
   active->s5, scale base/ptr->v1, zero-init loop ptrs v1/a0/v0/a1. EGC 2.95.2 from any tested
   C source: numpower->s4, numscale->s5, active->s3, scale base->a2, loop ptrs a2/t0/a0/t1.
   This single permutation cascades into all 256 diffs. Declaration reordering (active/i vs
   pointer locals) has NO effect; both scheduling controls and CSE flag matrix don't change
   Reload coloring. Hard-register pins force the names but re-color the other live ranges
   (frame usage, normalization cursors, wave-base register, switch bounds-check RTL), so they
   don't yield a clean match. The wave-base hoisting is coupled: original loads `a2=s6+lo`
   ONCE before the main loop and RELOADS it in the back-edge delay slot (0x1E9050); the
   candidate recomputes `addiu v1,s6,lo` inside the loop and hoists the `sll` — the opposite
   invariant motion, a consequence of the same coloring.
2. **Switch jump-table placement.** The original dispatch does `lui/addiu %hi/%lo(jtbl_001E7640)`
   (a symbol at 0x1E7640, inside the original `.data` blob; the linker script already defines
   `jtbl_001E7640 = 0x1e7640`). EGC emits the candidate's generated switch table into the
   candidate object's `.rdata` section, which links to a DIFFERENT address than 0x1E7640. So the
   dispatch `lui/addiu` words (0x1E8E24/0x1E8E2C) and the table location/content can't match
   without a linker experiment: split the original data blob around 0x1E7640 and place this TU's
   `.rdata` table into that exact hole, ensuring no second table remains in `.rdata`, and verify
   the relocated entries equal `0x1E8FE4, 0x1E8E40, 0x1E8E60, 0x1E8EC0, 0x1E8EF4, 0x1E8F34`.

Even with the linker hook, the Reload coloring (blocker 1) remains. There is no ordinary C
source or TU flag that resolves both. Retain INCLUDE_ASM; full-ELF parity preserved.

## Cross-checks

- Deadlocked (`reference/dl/game_dl/actuator.cpp` ~line 3364) confirms the structure and the
  case-1 `else pow=minpower` and the lifeSpan load-once form. DL differs: its struct has an extra
  `loop` char (20-byte stride) and it uses direct `(float)` casts where RC1 uses `func_001FA6C0`,
  and it takes an extra `PAD*` arg. Not directly usable for byte-matching.

## Future route (if revisited)

Linker/config experiment for the jump-table hole (blocker 2) is prerequisite but NOT sufficient;
the Reload coloring (blocker 1) would still need to be matched, which currently has no credible
strictly-C lever in EGC 2.95.2.
