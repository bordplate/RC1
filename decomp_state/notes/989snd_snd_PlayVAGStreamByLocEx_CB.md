# snd_PlayVAGStreamByLocEx_CB (0x12EC08) — BLOCKED: local EGC arg-copy fold + head schedule

Status: blocked 2026-09-20; re-investigated 2026-09-26 (best improved 9 word-diffs
-> 3 byte-diffs, i.e. 24/25 words correct). Kept as INCLUDE_ASM (boot parity intact).

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
scheduling it forces) is the dominant gap under DEFAULT flags.

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

## Best candidate (2026-09-26; 3 byte-diffs, NOT a match)
Supersedes the 2026-09-20 selective-volatile 9-diff form. With
`-fno-schedule-insns` (added to project defaults) and 4 hard-register pins, the
extra a1 copy is removed AND the head schedule matches, leaving ONLY the sub_group
load position wrong (24/25 words correct):

    int data[7];
    register int lo2 asm("$3");   // offset2&0xffff -> v1 (REQUIRED; else andi a3,a3)
    register int hi1 asm("$2");   // vol<<16        -> v0
    register int lo1 asm("$6");   // offset1&0xffff -> a2 (in place)
    register int sub asm("$12");  // sub_group      -> t4
    int hi2;
    lo2 = offset2 & 0xFFFF;
    hi1 = vol << 16;
    lo1 = offset1 & 0xFFFF;
    sub = sub_group;
    hi2 = pan << 16;
    data[2] = hi1 | lo1;
    data[3] = hi2 | lo2;
    data[0] = loc1;
    data[1] = loc2;
    data[4] = vol_group;
    data[5] = queue;
    data[6] = sub;
    snd_SendIOPCommandNoWrite(0x2C, 0x1C, (char*)data, cb, user_data);

The residual is a 3-word rotation of {sll v0 / andi a2 / lw t4}: the sub_group load
`lw t4,48(sp)` is hoisted to position 2 by the post-reload scheduler (sched2), but the
original has it at position 4 (between `andi a2` and `sll t1`). Everything from 12ec1c
onward (pack ors, the early sw a0 / move a2 / sw a1, the call setup
[li cmd, lw cb, li size, ld ud, sq ra], the late data[2..5] stores, the
jal + sw t4 delay slot, and the epilogue) is BYTE-IDENTICAL.

The 2026-09-20 selective-volatile form (`volatile int data[7]` +
`register int sg __asm__("$12")` + `(int*)data` casts + off1m/off2m locals) is the
default-flags best: 100 bytes, 9 word-diffs (-fno-schedule-insns2) / 13 (default).

## 2026-09-26 fix attempts for the lw t4 position (all failed)
- `-fno-schedule-insns2` (sched2 off) or both schedulers off: breaks the
  data[2..5] store grouping (they no longer land after the call setup) -> 20-21 diffs.
- Zero-byte barriers `asm volatile("" : "+r"(sub) : "r"(lo1))` and
  `asm volatile("" : : "r"(lo1))`: 24 diffs (disrupt allocation).
- `"memory"` clobber after lo1: 24 diffs (flushes ALL stack loads early).
- `+m`(sub_group) tied constraints (2026-09-26 last-resort recommendation):
  `asm volatile("" : "+m"(sub_group),"+r"(pan) : "r"(lo1))` -> 26;
  `"+m"(sub_group)` alone -> 24. Both hoist cb/user_data and mis-allocate.
- Explicit load asm `asm volatile("lw %0,48($sp)" : "=r"(sub) : "r"(lo1))`:
  places the sub load at the CORRECT position, but the asm barrier hoists the cb
  (`lw $13,56(sp)`) and user_data (`ld $14,64(sp)`) loads to positions 2/4 (orig:
  call setup) -> not a match. Note: this EGC build does NOT expand `%n` memory
  operands in extended asm (emits `lw $12,%2` literally), so the `"m"`-constraint
  form is unusable.
- Volatile load `sub = *(volatile int*)&sub_group;`: still hoisted (3 diffs).
- Pinning hi2 to $9: 7 diffs (worse). Dropping the sub pin: still 3. Minimal pins
  (lo2 only) + -fno-schedule-insns: 13 diffs (mis-allocation).
- Deadlocked store order [data[3],data[2],data[6],data[0],data[1],data[4],data[5]]
  with/without the lo2 temp: 24-25 diffs, does not change the pack-region ordering.

## Escalation (required before blocking)
2026-09-20:
- `expert` (one-shot GPT-6 Astra) invoked: recommended (1) a named-register
  `__asm__("$5")` idiom for data[1] and (2) `-fno-regmove`. Both tested:
  named-register -> 100B/17-diff (perturbs schedule); -fno-regmove -> 104B/23-diff.
- `last-resort-decompiler` (GPT-5.6 Sol) invoked: DISPROVED the "offset-4 copy is
  unavoidable" premise — found the selective-volatile route (100B, 9-10 diff).
2026-09-26:
- `decomp-researcher` invoked: found the -fno-schedule-insns + 4-pin form reducing
  to 3 diffs (the lw t4 position); confirmed the default-flags move-v1,a1 is robust;
  recommended the Splat-TU-split strategy if a flag deviation is accepted.
- `last-resort-decompiler` (GPT-5.6 Sol) invoked: prescribed the narrow
  `+m`(sub_group) tied-constraint barriers and an explicit-load asm as the
  discriminator, plus a default-flags volatile-store family as the only other hope.
  Applied and mechanically diffed: the `+m` forms regressed to 24-26 diffs (hoist
  cb/user_data); the explicit asm places the sub load correctly but hoists
  cb/user_data (not a match). Model's own verdict: if the narrow memory-dependency
  forms fail (they did), this is a concrete sched2 load-hoist tie-break with no
  default-flags pure-C form.

## Conclusion
The function cannot be matched with the local EGC/EEGCC 2.95.2 toolchain. Default
flags retain the offset-4 (data[1]) argument copy (combine-pass non-fold) that
Insomniac's build folded (22 diffs / 26 words). Forcing the fold via
`-fno-schedule-insns` + 4 pins gets to 24/25 words (3 byte-diffs) but the
post-reload scheduler (sched2) deterministically hoists the independent sub_group
load to position 2 instead of the original's position 4, and no C structure, pin,
barrier, or flag combination reproduces position 4 while keeping the rest of the
schedule. Retain INCLUDE_ASM; full-ELF parity preserved. last-resort GPT-5.6 Sol used.
