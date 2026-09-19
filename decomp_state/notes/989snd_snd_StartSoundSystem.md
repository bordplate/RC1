# snd_StartSoundSystem (0x12DA28, 0x258 = 150 words) — BLOCKED at 28 scheduling diffs

Target: `code/989snd/ee/989snd_pre.c` (the ONLY function in this TU, so per-TU flags
are safe). Binds the 989 IOP sound-server RPCs (0x123456 command, 0x123457 CD),
spinning until the IOP marks each client bound, resets CD callback state, then sends
the initial 4-byte CD stream-status command via `snd_SendIOPCommandAndWait(0,4,&data)`.

## Status
BLOCKED. Best candidate matches 122/150 words (28 linked word diffs). The two
spin-loop bodies (the originally-dominant 22-word gap) now match EXACTLY. The
remaining 28 diffs are all subtle instruction-ordering (scheduling) differences in
straight-line code (prologue, loop-2 setup, final block) plus two register
allocations. See "Remaining 28 diffs" below.

## Root-cause history (what was wrong and what fixed it)
1. **wait folding** — EGC 2.95.2 folds `wait - 1` (wait=10000) to `li v0,9999`;
   the original keeps `li s2,10000` + `addiu v0,s2,-1` computed at runtime in the
   `bgez` delay slot. A top-level `asm volatile("" : "+r"(wait))` barrier prevents
   the fold BUT, placed at init, triggers loop-invariant code motion (hoists
   wait-1 to the prologue) and flips `bgez`(0x07)→`bgezl`(0x17).
2. **Spin-loop padding (the dominant 22-word gap)** — the original pads EACH of the
   4 spin loops with 5 true nops (0x00000000): 2 error loops (`for(;;)`, 5 nops +
   self-branch + delay nop) and 2 countdown loops (`do{data--}while(...)`, 5-nop
   body + 1 pre-loop nop). My EGC emitted tight loops (~2 nops). NOTE: objdump
   COLLAPSES consecutive nops into "...", so the padding is invisible in objdump —
   count from the generated `.s` (25 nops total) or raw bytes, NOT objdump.
   Initially misdiagnosed as an unfixable older-compiler artifact. The
   last-resort-decompiler (GPT-5.6 Sol, invoked per hard rule) DISPROVED that:
   EGC 2.95.2 CAN reproduce the exact padding.

## The form that fixes the loop bodies (last-resort recommendation, applied)
- Separate per-phase waits (`wait1`, `wait2`), each materialized once before its
  loop (`li sN,10000`).
- The empty asm barrier placed IMMEDIATELY AFTER each bind call (inside the loop),
  NOT at init: `ret = sceSifBindRpc(...); asm volatile("" : "+r"(wait));`. This
  prevents the `wait-1` fold while keeping `bgez`(0x07) and no hoisting.
- `ret` is the countdown register; `data = ret` is assigned BEFORE the `if` (and
  again after the countdown) so the beq-delay-slot store is `sw v0,0(sp)`, not
  `li v1,-1`.
- Fixed-register locals to force the original's saved-register allocation:
  phase 1 `register int wait asm("$18")=10000; register int minus asm("$17")=-1;`
  (wait→s2, minus→s1); phase 2 `register int wait asm("$17")=10000; register int
  minus asm("$16")=-1;` (wait→s1, minus→s0). The `if` compares `ret != minus`
  (beq vs the saved -1); the `while` compares `ret != -1` (bnel vs `li v1,-1`).

This produces exactly 600 bytes / 150 words with both loop bodies byte-identical to
the original (verified: `bgez v0`(0x0441000c), delay `addiu v0,s2,-1`, 5-nop error
loop, `beq v0,s1`, delay `sw v0,0(sp)`, `li v1,-1`, `addiu v0,v0,-1`, 5-nop countdown
body, `bnel v0,v1`, delay `addiu v0,v0,-1`, `sw v0,0(sp)`, `lw v0,36(s5)`,
`beqz v0`; and the loop-2 analogue with s1/s0/s4).

## Remaining 28 linked diffs (straight-line scheduling + 2 allocations)
Measured by building the full image and comparing `build/boot_elf.elf` vs
`assets/boot_elf.elf` word-by-word over 0x12DA28..0x12DC7C:
- **Prologue (idx 21-32, 0x12DA7C-0x12DAA8)**: the register SAVE order is identical
  (s8,s0,s7,s6,s5,s4,s3,s2,s1,ra) but the INTERLEAVING of `lui`(addr-hi) / `sq`(save)
  / `li`(const) / `sw`(buffer store) differs. e.g. mine emits `lui s7` before
  `sq s4`; original emits `sq s4` first and `lui s7` later. Pure scheduler
  interleaving; not register allocation.
- **idx 42 (0x12DAD0)**: the loop-1 `-1` (`li s1,-1`) materializes at a different
  point relative to `lui s4`(srcFileHi).
- **Loop-2 setup (idx 83-88, 0x12DB74-0x12DB88)**: order of `li s1`(wait2),
  `li s0`(minus2), `lui s3`(errStrHi), `lui s2`(srcFileHi) differs.
- **Loop-2 error path (idx 96-97, 0x12DBA8/AC)**: REGISTER SWAP — original has
  errStr in s3 / srcFile in s2 (`addiu a0,s3,%lo(errStr); addiu a1,s2,%lo(srcFile)`);
  mine has errStr in s2 / srcFile in s3. (Loop 1 is correct in both: errStr=s3,
  srcFile=s4.)
- **Final block (idx 123-137, 0x12DC14-0x12DC4C)**: instruction reorder + a
  v0/v1 REGISTER SWAP. Original: `v0=&cdStreamInfo`, `v1=0xFFC`, then
  cmd1[0]=0(via hi-only s8), cd_busy=0, cmd2[0]=0, a2=sp, batchFree[1]=0xFFC(v1),
  a0=0, data=&cdStreamInfo(v0→0(sp)), cd_error=0(v0+0x10), jal, batchFree[0]=0xFFC
  (v1, in jal delay slot, GPREL -0x7F58(gp)). Mine swaps v0/v1 (0xFFC in v0,
  &cdStreamInfo in v1) and reorders the stores.

Flags tested (all made it WORSE or no help): `-fno-schedule-insns` (81 diffs),
`-fno-schedule-insns -fno-schedule-insns2` (84), `-mno-split-addresses`, `-G0`,
`-fno-gcse` (didn't stop the fold), `-fno-rerun-cse-after-loop` (grows to ~612 B,
breaks prologue). The original was built WITH the default scheduler, so
`-fno-schedule-insns` is the wrong direction.

## Why blocked
The two hard loop bodies now match exactly, but the straight-line prologue/setup/
final-block SCHEDULING (interleaving of independent lui/sq/li/sw) and two register
allocations (loop-2 errStr/srcFile; final-block v0/v1) do not match EGC 2.95.2's
default scheduler output. These are scheduler tie-breaks not controllable by the C
structure or the tested flags. A 600-byte candidate with exact loop bodies was
obtained but carries 28 straight-line scheduling diffs. Reverted to INCLUDE_ASM.

## Durable state
- pre.c reverted to `INCLUDE_ASM(...snd_StartSoundSystem)`.
- Best candidate C (the 28-diff form) is saved at
  /tmp/opencode/pre_best_28diffs.c for a future compiler/flag revisit.
- Structs/externs/symbols established by this investigation are correct and were
  kept in pre.c: `SndRpcServer` (bound at +0x24), `SndCdStreamInfo` (cd_error at
  +0x10, volatile — also the canonical definition in 989snd_post.c, whose fields
  were renamed state->cd_busy / error->cd_error and grown to the full 0x40 with
  pad_14[11]), and the buffer/array globals. `sceSifInitRpc`/`sceSifBindRpc` are
  named in config/symbols.txt. The SN `.extern` seed block used during the C
  investigation was removed: with the function back to INCLUDE_ASM it is unused,
  and 989snd_pre.o stays on the GNU assembler list.
