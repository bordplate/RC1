# snd_PlayVAGStreamByLocEx_CB (0x12EC08) — matched 2026-09-27

100-byte (25-word) 11-parameter wrapper in `code/989snd/ee/989snd_post.c` that
assembles the `SND_IOP_CMD_PLAY_VAG_STREAM` (0x2C) command payload and issues it
through `snd_SendIOPCommandNoWait` with the caller's completion callback.

## Signature and payload

```c
void snd_PlayVAGStreamByLocEx_CB(int loc1, int loc2, int offset1, int offset2,
    int vol, int pan, int vol_group, u32 queue, u32 sub_group,
    SndCompleteProc cb, u64 user_data);
```

28-byte (7-int) payload; each (value << 16 | offset) pair is packed into one word:
- data[0]=loc1, data[1]=loc2
- data[2]=(vol<<16)|(offset1&0xFFFF), data[3]=(pan<<16)|(offset2&0xFFFF)
- data[4]=vol_group, data[5]=queue, data[6]=sub_group

Semantics cross-checked against the Lombyte callers (11 args, e.g.
`fun_00215518.c`: `(val,0,0,0,arg2,0,2,0,0x21,cb,(u64)ptr)`) and the Deadlocked
descendant `snd_PlayVAGStreamByLocEx_CB` in `reference/dl/989snd/ee/989snd.c`
(12 args / 8-int payload; RC1 dropped the trailing `flags` field, hence 11 args /
7-int payload and data_size 0x1C instead of 0x20).

## Calling-convention notes

- Register arg window is a0-a5, t2, t3 (7th/8th int args in t2/t3 per the
  documented EGC 8-arg ABI). sub_group/cb/user_data are stack args.
- EGC 8-byte-aligns EACH stack argument, so cb lands at sp+0x38 and user_data at
  sp+0x40 regardless of sub_group's declared size. sub_group therefore works as a
  plain `u32` (a `u64` declaration made EGC load it with `ld`+`dsll32`+`dsra`
  instead of a single `lw`, growing the function to 112 bytes).

## Match-sensitive register pins

Local EGC 2.95.2 (default flags) got every buffer-value register right
(data[2]->$2, data[3]->$9, sub_group->$12, all call args, prologue) but lost two
tie-breaks, both fixed with register pins (a documented project technique; the
sibling `snd_SendIOPCommandNoWait` in this same file already pins registers):

1. `register int lo2 asm("$3"); lo2 = offset2 & 0xFFFF;` — the original masks
   offset2 into $3 (v1), keeping $7 (a3) reserved for the later `lw cb`. EGC
   otherwise masks in place into $7. Splitting data[3] into `hi2 | lo2` is what
   lets the mask be pinned.
2. `register int loc2arg asm("$5") = loc2;` — the original stores loc2 straight
   out of its a1 parameter register (`sw $5,4(sp)`) early. EGC otherwise copies it
   to $3 (`move $3,$5`) and defers the store, adding a word.

The `hi2` temporary and the pin-split of data[3] exist solely to carry these pins;
the data[2] expression stays inline.

## Verification

- `tools/decomp_probe.py` on the standalone candidate: `match: true` (100/100
  bytes, no differences), default flags `-G8 -O2 -ffast-math -fno-exceptions
  -snas`.
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf`: byte-identical.
- Function region 0x2FB88..0x2FBE4 (file) / 0x12EC08..0x12EC6B (vram) verified
  identical between the two ELFs.
- `tools/decomp_status.py --count`: 633 -> 632.

## History

- 2026-09-20: first decompiled attempt blocked — the combine pass retained an
  offset-4/data[1] a1 argument copy Insomniac folded (extra `move v1,a1`), 22
  diffs / 26 words under default flags, robust across ~30 C structures.
- 2026-09-26: re-investigated; best was 3 byte-diffs (24/25 words) via
  `-fno-schedule-insns` + 4 pins (lo2->$3, hi1->$2, lo1->$6, sub->$12). Every
  word matched except the sub_group load `lw t4,48(sp)`, which sched2 hoisted to
  position 2 (orig: position 4). Concluded "unfixable sched2 load-hoist tie-break".
- 2026-09-27 (block re-confirmed): full escalation (decomp-researcher -> expert
  GPT-6 Astra -> last-resort-decompiler GPT-5.6 Sol) re-prescribed the
  `-fno-schedule-insns` + 4-pin route; primary asm-load form = 4 diffs, plain
  `sub = sub_group` alternative = 3 diffs (best). Block retained.
- 2026-09-27 (UNBLOCKED, matched): a fresh `last-resort-decompiler` pass found a
  different, cleaner formulation — TWO register pins under the DEFAULT production
  flags (no scheduler flags): `lo2` pinned to $3 (original keeps $7 reserved for
  the later `lw cb`) and `loc2arg` pinned to $5 (direct early `sw` of the a1
  param, no copy). This route avoids the sched2 sub_group-load hoist entirely
  because the default scheduler already places that load correctly; it fixes the
  two residual tie-breaks (offset2 mask register and the deferred loc2 store) that
  the 4-pin scheduler-flag route left behind. Primary agent first exhausted the
  usual routes (u64/u32/int sub_group, buffer assignment orders, explicit
  temporaries, -fno-schedule-insns/-insns2/both), then applied and mechanically
  verified the two-pin recommendation. `decomp_probe.py` match:true (100/100),
  clean `make -B` + `cmp` byte-identical, independent `decomp-verifier` PASS.
  Stale blocked.json entry removed.
