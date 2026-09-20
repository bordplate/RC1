# snd_SendIOPCommandAndWait (code/989snd/ee/989snd_post.c) - MATCHED 2026-09-20

0x12E548, 404 bytes. Synchronous 989snd IOP command wrapper: stage the payload,
flush pending work, arm the return slots, issue the blocking RPC, spin for the
IOP return word, then flush a pending command batch. Returns the IOP return
word (`snd_syncBuffer[1]`).

## Resolution

Three codegen facts were required; each was isolated with probes
(`tools/decomp_probe.py ... --assembler=gnu`, final probe 404/404 bytes, 0 diffs,
then full `make` + `cmp` byte-for-byte):

1. **Copy loop must be a plain `for`, not `if (count > 0) { for ... }`.**
   The wrapped form makes EGC emit a DOUBLE `blez` and hoist an s5 `la` of
   `snd_batchCommandBuffers`, growing the frame 0x70 -> 0x80. The original's
   guarded do-while (single `blez`) is reproduced exactly by the unwrapped
   `for (i = 0; i < count; i++) snd_syncSendBuffer[i] = data[i];`.

2. **The return word must be loaded into a local AFTER the spin loop.**
   `ret = snd_syncBuffer[1]; return ret;` makes EGC keep the array base in the
   saved s0 (hi/lo split: `addiu v1, s3, 0x3100` in the loop delay slot,
   `lw s0, 4(v1)`), then `move v0, s0` at both exits. A bare
   `return snd_syncBuffer[1];` instead loads directly into v0 at the very end;
   loading inside the do-while body (the Deadlocked/Ghidra shape) hoists a
   materialized base pointer (`lui/addiu` before the loop). Neither matches.

3. **The spin loop body is padded with three explicit nops in the original**
   (`jal snd_GotReturns; nop; nop; nop; nop; beqz ...`). EGC emits none of
   them: its asm is `jal; beq (in the jal delay slot)`, and the EE assembler
   inserts only ONE nop in the jal delay slot, leaving a single-nop body.
   `asm volatile("nop\n\tnop\n\tnop")` inside the do-while body supplies the
   three; the assembler's own delay-slot nop makes four, matching the original.
   Same precedent as `moby_getActiveObject` / `moby_getSecondaryObject` in
   code/game/mobyutil.cpp (three-nop `asm volatile` barrier).

## Supporting details

- Prototype is `unsigned int (int cmd, int count, char* data)` (C linkage,
  989snd is C). Deadlocked's 989snd build confirms `u_int` return. The
  previous `void / void*` prototype in 989snd_post.c:3 and 989snd_pre.c:36 was
  wrong; `snd_SendIOPCommandNoWait` stays `void`. Callers pass `int[N]`
  buffers or 0 (harmless incompatible-pointer warnings), and all seven matched
  callers plus the two blocked callers (snd_StartSoundSystem 0x12DC48,
  snd_InitVAGStreamingEx 0x12EBA4, which stores the return into
  `snd_cdStreamActive`) still match with the corrected prototype.
- `SND_SYNC_RPC_RESULT_SIZE` (0xC) names the RPC result-region argument: the
  three return words validated by snd_GotReturns.
- New named globals in config/symbols.txt: `snd_syncBuffer` (0x133100,
  16-word sync return area), `snd_syncSendBuffer` (0x133140, 0x200-byte
  staging buffer).
- EGC 2.95.2 rejects C99 mid-function declarations; `i`, `r`, `ret` are all
  declared at the top.

## Verification

```sh
source .venv/bin/activate
python3 tools/decomp_probe.py <probe> \
  code/_generated/nonmatchings/989snd/ee/989snd_post/snd_SendIOPCommandAndWait.s \
  snd_SendIOPCommandAndWait --assembler=gnu   # match: True, 404/404, 0 diffs
make -j2 && cmp build/boot_elf.elf assets/boot_elf.elf   # byte-for-byte
```

The reference `.s` moved from nonmatchings to matchings.
