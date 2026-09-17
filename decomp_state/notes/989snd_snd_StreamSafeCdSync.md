# snd_StreamSafeCdSync (0x12EE08, 156 bytes) — MATCHED

Stream-safe CD sync helper. If no stream-safe session is active
(`snd_cdStreamActive == 0`), delegates to `func_00120C30(arg0)` (SCE command-queue
state check). Otherwise it flushes the cache, snapshots
`stateZero = (snd_cdStreamInfo.state == 0)`, stores it to
`snd_cdStreamEndPending`, returns 0 if the stream is already empty, returns 1 if
`arg0 == 1`, then busy-waits (`snd_FlushSoundCommands` + `FlushCache(0)` per
iteration) until `snd_cdStreamInfo.state != 0`, storing the pending flag on each
pass, and returns 0.

Declared in `code/989snd/ee/989snd.c` (GNU-compat TU). Globals:
`snd_cdStreamActive` (0x15EC8C, GPREL), `snd_cdStreamEndPending` (0x15EC98,
GPREL), `snd_cdStreamInfo` (0x137B00, `volatile SndCdStreamInfo`, absolute
self-based load). Callee `func_00120C30` receives the argument in a0 (the call
site has a `nop` delay slot, so a0 is left as the caller's parameter).

## The two codegen facts that make it match

1. **`snd_cdStreamInfo` must be a `volatile` struct.** The out-of-window 0x137B00
   base load must not be CSE'd/folded; the `state` field is at offset 0.
   (`extern SndCdStreamInfo snd_cdStreamInfo;` where the struct is `volatile`.)

2. **The post-loop `snd_cdStreamEndPending = stateZero` store goes INTO the
   do-while body, not after the loop.** Writing it as a statement after the
   `do { ... } while (...)` puts it after the back-branch with a `nop` in the
   back-branch's delay slot (1 insn too long). Placing it inside the loop body
   makes EGC schedule it into the back-branch's delay slot (0x12EE8C), matching
   the original's per-iteration store.

## The expert insight that fixed the final 2 diffs

After the two facts above, the candidate was 156 bytes with 2 diffs, both in the
dead front-check region:

- 0x12EE60: `bnez v0, TARGET` — original targets the epilogue `lq ra` (0x12EE94);
  the candidate targeted `lq s0` (0x12EE98).
- 0x12EE64: original is `move v0, zero`; the candidate emitted an early `lq ra`.

The dead front-check is `if (stateZero) return stateZero;` (unreachable, since
`stateZero == 0` there after the earlier `if (stateZero == 1) return 0;`).
EGC retains the `bnez`. The root cause (expert, GPT-6 Astra): the dead path's
**return expression** controls the delay-slot fill. `return stateZero` needs no
return-value instruction, so EGC copies the epilogue's `lq ra` into the delay
slot and advances the taken-target past it. Changing the dead path to
`return 0;` gives EGC a zero-return instruction to fill the delay slot, and the
`bnez` then targets the common `lq ra`:

```c
    if (stateZero == 1)
        return 0;
    if (arg0 == 1)
        return 1;
    if (stateZero)
        return 0;          // dead, but must return 0 (not stateZero)
    do { ... } while (stateZero == 0);
    return 0;
```

This is a general lesson: when a retained-but-dead branch has a missing/extra
delay-slot instruction, vary the branch's return expression (literal vs. the
tested variable) — it changes which instruction EGC copies into the delay slot
and where the taken-target lands.
