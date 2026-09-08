# snd_PostMessage (code/989snd/ee/989snd.c) - MATCHED 2026-09-08

```c
void snd_PostMessage(void) {
    int* p = snd_batchCommandBuffers[snd_batchIndex];
    (*p)++;
    snd_FlushSoundCommands();
}
```

## What it does

Increments the command count of the currently active sound batch buffer
(`*snd_batchCommandBuffers[snd_batchIndex]`, buffer ptrs at 0x15ECA0, count at
the buffer head) and then flushes the pending IOP sound command queue via
`snd_FlushSoundCommands` (0x12DC80). Called from `snd_SendIOPCommandNoWait`
(0x12E968) right after appending a command record to the batch — the
"post one message" step of the 989snd IOP batching layer. Index global is
`snd_batchIndex` at 0x15ECC0 (gp-0x7F40).

## Codegen notes

- The matching form is `(*p)++; snd_FlushSoundCommands();` with the pointer
  local `p`. Everything (idx load, table walk, count load/increment, count
  store) completes BEFORE the call, so no register survives the call and the
  frame is the minimal 0x10 with only `sq/lq ra` at 0(sp).
- EGC then schedules the count store `sw v0,0(a0)` into the `jal` delay slot,
  exactly as in the original.
- The alternative form `int v = *p + 1; snd_FlushSoundCommands(); *p = v;`
  FAILS: EGC must keep `p` (a0) alive across the call, grows the frame to
  0x30 and saves $16/$17 — 20+ extra words past the 56-byte original.
- `snd_batchIndex`/`snd_batchCommandBuffers` are plain externs (no section
  attribute); both addresses are inside the gp window, so EGC emits the
  GP-relative `lw v1,-0x7F40(gp)` (hoisted above the frame setup) and the
  single `addiu v0,gp,-0x7F60` array base. Same idiom as the matched
  `snd_SendCurrentBatch` in this file.
- Default compiler flags; no per-file or per-function flag changes.

## Verification

- Probe `decomp_state/probes/snd_postmessage_v2.c` vs generated
  `snd_PostMessage.s`: 56/56 bytes, 0 differences (v1 with the explicit-`v`
  form kept as the known-bad shape reference).
- Built object objdump of 0x12E9A0..0x12E9D8 is byte-identical to
  assets/boot_elf.elf.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf`: identical.
  Count 742 -> 741.
