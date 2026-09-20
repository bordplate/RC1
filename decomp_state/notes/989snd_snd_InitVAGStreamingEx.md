# snd_InitVAGStreamingEx (code/989snd/ee/989snd_post.c) - MATCHED 2026-09-20

0x12EB20, 172 bytes. Initializes the IOP VAG streaming session. If
`snd_cdStreamActive == 1` (session already initialized) it returns 0 without
touching state; otherwise it drains pending CD callbacks by spinning on
`snd_FlushSoundCommands` until the flush result is idle, calls
`snd_StreamSafeCdSync(0)`, stages the four parameters
(`num_channels`, `buffer_size`, `read_mode`, `enable_streamsafe`) in a
stack buffer, issues command 0x2A (16-byte payload) through
`snd_SendIOPCommandAndWait`, and stores the IOP response back into
`snd_cdStreamActive`, which it also returns.

Deadlocked (reference/dl/989snd/ee/989snd.c, `gStreamingInited` /
`gLoadBusy`) has the same body and signature
`int (int, int, unsigned int, int)`.

## Resolution

- Drain loop form: the original tests `snd_cdCallbackPending` ONCE at loop
  entry (`lw v0; beqz; nop`) and the back edge reuses the
  `snd_FlushSoundCommands` return value (`jal; nop x4; bnez v0; nop`) — there
  is NO reload at the back edge. Plain `while (snd_cdCallbackPending != 0)
  snd_FlushSoundCommands();` makes EGC reload the global at the back edge
  (extra `lw`, +1 word, wrong shape). The matching form keeps the condition
  in a register:

  ```c
  pending = snd_cdCallbackPending;
  while (pending != 0) {
      pending = snd_FlushSoundCommands();
      asm volatile("nop\n\tnop\n\tnop\n\tnop");
  }
  ```

- Loop-body padding: the original body is `jal + 4 nops + bnez + nop`.
  Under the GNU macro assembler (989snd_post.o TU), the first explicit APP
  nop is consumed as the jal delay-slot filler, so FOUR explicit nops are
  needed here. This differs from the snd_SendIOPCommandAndWait precedent
  (three explicit nops), where the back-edge `beq` sits in a `.set
  noreorder` region and the assembler inserts one extra nop. Calibrated with
  probe .s/.o byte dumps (probe_c3: jal+3 nops, short by one word, shifts the
  rest of 989snd_post.o and mis-links cross-TU jals to
  snd_StreamSafeCdSync; probe_c4: exact shape).

- Prologue: the `snd_cdStreamActive` load is hoisted to the prologue head
  (word 1), `li v0,1` interleaved after the first `sq`, and the `s0 = a0`
  param move scheduled into the `bne` delay slot — all fall out of the plain
  `if (snd_cdStreamActive == 1) return 0;` form with four int params.

- The `move` pseudo (0x0000202D) is the original's encoding for all
  register copies here; the Splat "daddu r,r,0" mnemonics in the generated
  .s are the reversed-byte (LE) hex fields misread (Splat comment quirk),
  not fixed-opcode daddus.

## Verification

Probes (`make probe ASSEMBLER=gnu`) matched the original word-for-word once
the loop form and nop count were fixed; final full `make` +
`cmp build/boot_elf.elf assets/boot_elf.elf` byte-for-byte. Added externs
`snd_cdStreamActive` / `snd_cdCallbackPending` / `snd_StreamSafeCdSync(int)`
to the 989snd_post.c header block (the pre-existing
`snd_cdStreamActive` declaration in the SndCdStreamInfo block was removed as
a duplicate). No new symbols needed: both globals already exist in
config/symbols.txt (0x15EC8C, 0x15ECC8).
