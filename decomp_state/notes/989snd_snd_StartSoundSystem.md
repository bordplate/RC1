# snd_StartSoundSystem (code/989snd/ee/989snd.c) - BLOCKED 2026-09-16

`void snd_StartSoundSystem(void)` at vram 0x12DA28, 0x258 bytes (600 bytes / 150 words),
frame 0xB0 (saves s0-s8 + ra).

## What the function does
Sound-system init. Writes the six sound-buffer base addresses into six global
pointer slots, calls SIF init (`func_0011AB20(0)`), then in two do/while-retry
loops binds two RPC clients (`func_0011AFF8`) for ids 0x123456 ("main") and
0x123457 ("cd"), busy-waiting until each client's `ready` field (struct offset
+0x24) is nonzero. On bind failure it prints an error
(`func_00116078(errString, srcFile, line)`) and spins forever. Between the two
bind loops it zeroes four cd-callback globals. Finally it zeros the head of both
command buffers, zeros `cdStreamInfo.state`/`.error`, sets two free-byte counters
to 0xFFC, sets a local `buf = (int)&cdStreamInfo`, and calls
`snd_SendIOPCommandAndWait(0, 4, &buf)`.

## Key structural facts (verified from the reference)
- Prologue materializes the six buffer bases into v0,v1,a1,a2,a3,t0 and stores
  them: the three `*1` slots (commandBuffer1/returnBuffer1/streamBuffer1 at
  0x15ECA0/0x15ECB8/0x15ECB0) are GPREL16 (`sw reg,off(gp)`), the three `*2`
  slots (0x15ECA4/0x15ECBC/0x15ECB4) are ABSOLUTE (`lui $at,%hi; sw reg,%lo($at)`).
  s8 = hi(commandBuffer1) is saved and kept live, used ONLY in the final
  section (`sw 0,%lo(commandBuffer1)(s8)` for `commandBuffer1[0]=0`).
- Each of the four loop bodies (2 fatal-error `for(;;)` + 2 delay spin) contains
  exactly 5 explicit NOPs. The 40 bytes of the 48-byte shortfall vs an empty-body
  candidate are these NOPs.
- Delay-loop shape (both loops): `i = 9999` (set as `addiu v0,saved10000,-1` in the
  bind `bgez` delay slot); top `beq i,-1` guard with `sw i,0(sp)` (entry store) in
  its delay slot; `v1 = -1`; a pre-decrement `addiu i,i,-1`; then the spin
  `5 nops; bnel i,-1,spin` with the decrement `addiu i,i,-1` IN THE bnel DELAY
  SLOT; then `sw i,0(sp)` (exit store). `buf` (sp+0, the address-taken local later
  passed to the IOP call) is the same variable as the counter.

## Progress made (this session)
A last-resort-decompiler (GPT-5.6 Sol) consultation corrected two errors and gave
concrete fixes that were implemented and verified with `tools/decomp_probe.py`:
1. The GP addresses were miscomputed earlier (gp-0x7F60 = 0x15ECA0, not 0x15EC00);
   the buffer slots ARE the three `[2]` arrays (snd_batchCommandBuffers /
   snd_batchReturnBuffers / snd_streamBuffers) — no six-independent-global model.
2. The 5-NOP loop bodies (`while/for { asm nop x5 }`) — added.
3. Drop the `commandBuffer1` local pointer (use the global directly) so EGC CSEs
   hi(commandBuffer1) into s8 — added; the prologue now matches and the frame is
   0xB0.
4. `snd_SendIOPCommandAndWait` returns `int` (Ghidra: returns DAT_00133104), not void.
5. `snd_cdCallbackArg` needs `__attribute__((section(".data")))` for its absolute
   `lui $at / sw` store; `snd_cdCallbackData` (long, 8-byte `sd`) stays plain.

With the `for (buf=10000; buf!=-1; buf--) { 5 nops; }` delay form and three
separate `.data`-section symbols for the `*2` slots (so the `[1]` stores are
absolute), the candidate reaches 600 bytes (size matches) but 95 word diffs remain.

## The two blockers (both EGC codegen, not steerable from C after many variants)
1. **`*2`-slot register allocation.** The original stores the `*2` values (v1/a2/t0)
   through `$at` (`lui $at,%hi(slot); sw reg,%lo($at)`), keeping the six bases in
   v0,v1,a1,a2,a3,t0. Declaring the `*2` slots as separate `.data` symbols makes
   EGC keep their addresses in BASE registers (sw reg,0(base); t0/a0/t1/t2...) and
   re-materializes the values into a different register set, shifting the whole
   prologue (38-word run at 0x12DA34-0x12DAC8). The plain `[2]`-array form keeps the
   prologue register set but emits the `[1]` stores as GPREL16 (576-592 bytes, size
   mismatch). Neither reproduces the original's `[0]`-GPREL + `[1]`-absolute-`$at`
   mix with the v0,v1,a1,a2,a3,t0 value allocation.
2. **Delay-spin scheduling.** The original uses `bnel` (branch-likely) with the
   countdown decrement in the branch's DELAY SLOT, plus a separate pre-decrement
   before the spin. EGC 2.95.2 (project flags) consistently emits `bne` with the
   decrement as a normal instruction BEFORE the branch (nop delay slot) and folds
   away the pre-decrement. Tried: `while (buf--) {nops}`, `buf--; while(buf!=-1){nops;buf--}`,
   `for (buf=10000; buf!=-1; buf--) {nops}`, and `-fno-schedule-insns2` — all give
   the `bne` form, none the `bnel`-delay-slot form.

## Attempts (all in /tmp/opencode/start_probe)
- Direct globals, all-local-pointers, cmdBuf1-local-only, `while(buf--)`,
  pre-decrement `while`, `for` loop, `-fno-schedule-insns2`: 524-592 bytes, no match.
- `.data`-section `*2` symbols + `for` loop: 600 bytes (size matches), 95 diffs
  (prologue alloc + bnel + middle/final).
- Isolated delay-loop probes (dA-dF, g1-g5) and the assembler expansion of
  `sym` vs `sym+4` (GNU as emits GPREL16 for both — the last-resort "symbol+4 →
  absolute" explanation does not hold under this toolchain).

## Last-resort result
last-resort-decompiler (GPT-5.6 Sol) was invoked for this exact target. It
corrected the GP-address arithmetic and the `snd_SendIOPCommandAndWait` return
type, and supplied the 5-NOP-body + no-commandBuffer1-local + `.data`
cdCallbackArg forms, all of which were implemented and reduced the diff from 143
to 95 words (size now matches). It did not identify a C form for the `[1]`-slot
`$at` allocation or the `bnel` delay-slot spin; its "symbol+4 expands absolute"
claim was disproven by the GNU-as relocation output (both `sym` and `sym+4` are
R_MIPS_GPREL16).

## Blocker
EGC 2.95.2 cannot be steered (via C form, `.data` section, or `-fno-schedule-insns2`)
to reproduce (a) the prologue's v0,v1,a1,a2,a3,t0 value allocation with the `*2`
slots stored through `$at` absolute (vs the `*1` slots GPREL16), and (b) the
delay-spin's `bnel` branch-likely with the decrement in the delay slot (EGC emits
`bne` with the decrement before the branch). Size matches at 600 bytes; 95 word
diffs remain across the prologue allocation, both delay spins, and the
middle/final sections. Revisit if the EGC build/flags are revisited.
