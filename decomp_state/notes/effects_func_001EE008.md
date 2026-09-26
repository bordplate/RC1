# func_001EE008 (code/game/effects.cpp) — BLOCKED

## Outcome: BLOCKED (44/796-byte diffs remain, all in call-setup emit order)

## What was solved (matches original byte-for-byte)
- Semantics 100% correct: `void func_001EE008(float x, float y, EffectSprite* spr)`.
  EffectSprite (48B): posX/posY/posZ(0) pad(0xC) scale(0x10f) data(0x14i)
  texId(0x18i) angle(0x1Cf) state(0x20i) field_24(s16) count(s16,0x26)
  angleStep(0x28f) type(0x2Ci).
- Frame size 0x90 (8-float-slot area, offsets at 0/4/16/20, 8/12/24/28 reserved)
  — REPRODUCED by `float offsetA[4]; float offsetB[4];` (two 4-float vectors).
- If-chain — REPRODUCED by `switch(type){case0/1/2}` (fully-branched, beq-to-tail).
- Prologue matches through 0x54; core FP regs (C->f24,x->f22,y->f23,angle->f21,z->f20).
- type1 four-draw quad + spin loop + type2 draw; type1 offset stores scheduled into
  trig-jal delay slots; candidate compiles to EXACTLY 796 bytes (== original).
- Draw callee func_001F5AB0: 7 floats (f12-f18) + 7 ints (a0-a3,t0-t3).
  z=0.0 (case0/1, via home reg f20), z=0.5 (case2, lui at,0x3f00).

## The BLOCKER (44 diffs, consistent across all 7 draw calls)
For each `func_001F5AB0` call, two instructions are fixed and MATCH: `f15=C*scale`
(0xcc) and the `jal` (0xdc). The other 8 arg-setup instructions are PERMUTED:
  ORIGINAL:  a0,a1,a2,a3(ori), [f15], t1,t2, f17, jal, [delay f18]
  CANDIDATE: f17,f18,         a0,a1, [f15], a2,a3(ori), t1, jal, [delay t2]
EGC emits the float z-arg moves (mov.s f17/f18, f20) FIRST in the candidate but LAST
(f17 before jal, f18 in delay slot) in the original. a2/a3/t2 are correspondingly
rotated. The RTL is IDENTICAL (same f20-home + mov.s form); only the EMIT ORDER differs.

## Why it's blocked — routes exhausted (all mechanically tested)
- `volatile float[4]` arrays  -> wrong (NOPS in trig delay slots; stores not scheduled in)
- `float[4]` plain arrays     -> WINNING frame+stores, but 44 emit-order diffs remain
- explicit `float z=0.0f` local -> WORSE (780 B, 187 diffs)
- z-args moved to LAST signature positions -> WORSE (832 B, 205 diffs)
- `-fno-schedule-insns` / `-fno-schedule-insns2` -> no effect (f17/f18 still top)
  => proves it is the RTL EMIT ORDER, not the instruction scheduler.
- EXPERT (GPT-6 Astra) tied-output float asm consuming the int-arg registers
  (`asm volatile("":" +f"(lateZ): "r"(p0..p6))`) -> WORSE (55 diffs): it moved the
  float args (f14/f12/f13/f16) to LATE and int args to FIRST (opposite of original).

## Escalations used (requirement satisfied)
- last-resort-decompiler (GPT-5.6 Sol): its "two 4-float vectors + switch" hypothesis
  was CORRECT and dropped 184 -> 44 diffs.
- expert (GPT-6 Astra): tied-float-asm suggestion tested, made it worse.

## Best candidate
decomp_state/notes/effects_func_001EE008.cand.cpp  (probe -> 44 diffs, 796 B)

## Resuming
If a future EGC/flag discovery reorders float-vs-int call-arg emission, re-probe
the durable candidate first:
  source .venv/bin/activate && python3 tools/decomp_probe.py \
    decomp_state/notes/effects_func_001EE008.cand.cpp \
    code/_generated/nonmatchings/game/effects/func_001EE008.s \
    func_001EE008__FffP12EffectSprite --define func_001F5AB0=0x1f5ab0 \
    --out working/effects_func_001EE008/outN
Then full build + `cmp build/boot_elf.elf assets/boot_elf.elf`.
