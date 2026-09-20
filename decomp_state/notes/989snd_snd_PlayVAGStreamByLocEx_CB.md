# snd_PlayVAGStreamByLocEx_CB (0x12EC08) — BLOCKED: local EGC arg-copy fold + head schedule

Status: blocked 2026-09-20. Kept as INCLUDE_ASM (boot parity intact).

## Target
`code/989snd/ee/989snd_post.c:278` — `snd_PlayVAGStreamByLocEx_CB`, 0x12EC08,
100 bytes / 25 words. Builds a 7-element int `data` array on the stack and calls
`snd_SendIOPCommandNoWait(0x2c, 0x1c, (char*)data, cb, user_data)`.

- data[0]=loc1(a0), data[1]=loc2(a1), data[2]=(vol<<16)|(offset1&0xffff) in v0,
  data[3]=(pan<<16)|(offset2&0xffff) in t1, data[4]=vol_group(t2),
  data[5]=queue(t3), data[6]=sub_group(t4, arg9). ra saved at sp+0x20.
- Args 1-8 in a0..t3; arg9(int)=[sp+0x30], arg10(cb)=[sp+0x38], arg11(u64)=[sp+0x40].

Deadlocked reference (12-param/8-elem sibling, `reference/dl/989snd/ee/989snd.c:1002`)
confirms the C shape: `data[3]=pan<<0x10|offset2&0xffffU; data[2]=vol<<0x10|offset1&0xffffU;
data[6]=sub_group; data[7]=flags; data[0]=loc1; data[1]=loc2; data[4]=vol_group;
data[5]=queue;`. RC1 is the 11-param/7-elem variant (no `flags`, data_size 0x1c).

## Root cause 1 — the offset-4 argument copy is not folded by local EGC
EGC copies every register argument into a pseudo at function entry
(r116=a0, r117=a1, r118=a2, r119=a3, r120=t0, r121=t1, r122=t2, r123=t3); the
COMBINE pass then substitutes the hard register back into the single use and deletes
the copy. Local EGC folds a0(data[0],off0), a2/a3/t0/t1(ALU), t2(data[4],off0x10),
t3(data[5],off0x14) — but NOT a1(data[1], offset 4): the copy `r117=a1` is kept and
the store reads it, emitting an extra `move $v1,$a1` + `sw $v1,4(sp)`. The original
stores `data[1]` as a DIRECT `sw $5,4(sp)`. That one extra instruction (and the
scheduling it forces) is the dominant gap.

Minimal-repro proof (all local EGC -G8 -O2, /tmp/opencode/pv_batch/minirepro{,2,3}.c)
that the offset-4 (index-1) store is the invariant non-fold, independent of statement
order, register, array size, or a `tmp` local:
- data[0]=X;data[1]=Y  -> X folds, Y(offset4) copy kept, for every X,Y in
  {a0,a1},{a2,a3},{t0,t1},{t2,t3}
- 7-elem data[0]=a0;data[1]=a1;data[4]=t2;data[5]=t3;data[6]=s9 -> a0,t2,t3 fold;
  a1(offset4) kept  [== real function]
- `int tmp=a1; data[1]=tmp;` -> same, a1 copy kept
Combine/RTL/sched dumps: /tmp/opencode/probe_dl.c.{rtl,cse,cse2,combine,sched}.
A matched sibling (snd_SetSoundParams_CB) also keeps a1/a2/a3 copies — consistent
local EGC behavior; the original's all-folded form is not reproducible with this build.

## Root cause 2 — head schedule / in-place andi
Even after removing the copy (below), the local EGC schedules the first ~16
instructions differently from the original:
- original: andi v1,a3 | sll v0 | andi a2 | lw t4 | sll t1 | or v0 | or t1 |
  sw data0 | daddu a2,sp | sw data1 | li a0 | lw cb | li a1 | ld | sq ra | ...
- local:    sq ra | andi a3(in-place) | andi a2 | sw data0 | sll v0 | sll t1 |
  lw t4 | or v0 | or t1 | sw data1 | li a0 | lw cb | li a1 | ld | daddu a2,sp
Differences: (a) `sq ra` is emitted in the prologue (pos 2 under -fno-schedule-insns2)
vs scheduled in the body (pos 16) in the original; (b) `sw data0` early vs late;
(c) `daddu a2,sp` late vs early; (d) the `offset2 & 0xffff` andi is computed
IN-PLACE in a3 locally but into v1 in the original (a register-allocation choice);
(e) the data[2]/data[3] sll/or interleaving differs.

## Best candidate (not a match)
Selective-volatile array + named register for sub_group (found by last-resort-decompiler
2026-09-20). Making data[0],data[1] (and one of data[2..5]) `volatile` while storing
the rest through a `(int*)data` cast removes the a1 copy WITHOUT adding an
instruction; `register int sg __asm__("$12")=sub_group` + `((int*)data)[6]=sg` keeps
the data[6] store able to fill the call delay slot. `int off1m=offset1&0xffffU;
int off2m=offset2&0xffffU;` locals trim 1 more diff.

    volatile int data[7];
    register int sg __asm__("$12") = sub_group;
    int off2m = offset2 & 0xffffU;
    int off1m = offset1 & 0xffffU;
    data[0] = loc1;
    data[1] = loc2;
    data[2] = vol << 16 | off1m;
    ((int*)data)[3] = pan << 16 | off2m;
    ((int*)data)[4] = vol_group;
    ((int*)data)[5] = queue;
    ((int*)data)[6] = sg;
    snd_SendIOPCommandNoWait(0x2c, 0x1c, (char*)(int*)data, cb, user_data);

Result: 100 bytes (size correct, move gone, tail sw data2..epilogue byte-exact),
**9 word-diffs** under `-fno-schedule-insns2`, 13 under default flags. All remaining
diffs are in the head (root cause 2). This is NOT a match and is not committable.

## Escalation (required before blocking)
- `expert` (one-shot GPT-6 Astra) invoked: recommended (1) a named-register
  `__asm__("$5")` idiom for data[1] and (2) `-fno-regmove`. Both tested:
  named-register -> 100B/17-diff (perturbs schedule); -fno-regmove -> 104B/23-diff.
- `last-resort-decompiler` (GPT-5.6 Sol) invoked: DISPROVED the "offset-4 copy is
  unavoidable" premise — found the selective-volatile route above (100B, 10-diff
  default / 10-diff -fno-schedule-insns2; 9 with the off1m/off2m locals). Confirmed
  no candidate matched; recommended the blocker wording if the volatile route is
  exhausted. It is now exhausted (~20 statement orders, int/u32 queue, DL order,
  unsigned casts, all scheduler flags).

## Conclusion
The function cannot be matched with the local EGC/EEGCC 2.95.2 toolchain: the
combine pass retains the offset-4 (data[1]) argument copy that Insomniac's build
folded, and even forcing the fold (selective volatile) leaves a head-schedule and
in-place-andi register-allocation difference of 9-13 words. Kept as INCLUDE_ASM.
last-resort GPT-5.6 Sol used.
