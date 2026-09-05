# snd_ResetMovieSound (0x0012F0A8) / snd_GetMovieNAX (0x0012F178)

## What they do

Both are one-call wrappers around the 989snd IOP command channel, in the
same family as the already-matched `snd_CloseMovieSound` (cmd 0x3C):

```c
void snd_ResetMovieSound(void) {
    snd_SendIOPCommandAndWait(0x3D, 0, 0);
}

void snd_GetMovieNAX(void) {
    snd_SendIOPCommandAndWait(0x5B, 0, 0);
}
```

`snd_SendIOPCommandAndWait` (0x12E548, still INCLUDE_ASM) sends an SIF command
to the 989snd IOP plugin and waits. 0x3D resets the movie-sound subsystem
(called by `audioDecReset__FP9_AudioDec`), 0x5B fetches the current movie
NAX timing value (called by `sendADPCM__FP9_AudioDec`).

## Note on the return value

`snd_SendIOPCommandAndWait` is declared `void` in 989snd.c, but the
`sendADPCM` caller consumes v0 after `jal snd_GetMovieNAX`
(`subu v0, v0, lw(s0,0x60)`) — it uses whatever the callee left in v0.
Writing the wrapper as `void` (matching the declaration) leaves v0
unmodified after the tail call, so the machine behavior is identical to the
original regardless of the declared return type.

## Codegen

Exact `snd_CloseMovieSound` shape: 0x10 frame, `li a0,CMD`, `sq ra`,
`move a1,zero`, `jal`, `move a2,zero` in the delay slot, `lq`, `jr`,
`addiu sp`. The jal relocates against the `.text` section symbol (callee
defined later in the same TU via INCLUDE_ASM — the documented EGC
same-section mechanism).

## Verification

- Objects: 10/10 words identical for each function; R_MIPS_26 resolves to
  0x12E548 (matches symbols.txt and the .s jal field).
- Full `make` + `cmp`: identical. Count 801 -> 799. decomp-verifier MATCH
  for both.
