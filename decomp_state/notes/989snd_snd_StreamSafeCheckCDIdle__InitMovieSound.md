# snd_StreamSafeCheckCDIdle (0x0012ED30) / snd_InitMovieSound (0x0012F068)

## What they do

Both marshal arguments into a stack buffer and send it to the 989snd IOP
plugin via `snd_SendIOPCommandAndWait(cmd, count, data)` (0x12E548):

```c
void snd_StreamSafeCheckCDIdle(int arg) {
    int buf[4];
    buf[0] = arg;
    snd_SendIOPCommandAndWait(0x36, 4, buf);
}

void snd_InitMovieSound(int a, int b, int c, int d, int e, int f) {
    int buf[6];
    buf[0] = a; ... buf[5] = f;
    snd_SendIOPCommandAndWait(0x3B, 0x18, buf);
}
```

- CheckCDIdle (cmd 0x36, 4 ints, only buf[0] set): poll the IOP stream plugin
  for CD idle. Callers: music_Stop (arg 1), func_0022D708 (arg 1, then
  snd_StreamSafeCdSync).
- InitMovieSound (cmd 0x3B, 6 ints): initialize the movie-sound subsystem.
  Caller audioDecCreate passes (0x400, 0x1000, 0x400, 0, 5, 3) — buffer/sizes
  for the movie ADPCM channels.

## Return value propagation

Both are declared `void` (matching snd_SendIOPCommandAndWait's void
prototype), but callers consume v0 after the call (func_0022D708: `s2 = v0`;
audioDecCreate stores v0/v1 into AudioDec+0x48/0x4C). The tail call to
snd_SendIOPCommandAndWait is the last action in each body, so v0/v1 at `jr ra`
are exactly the callee's return registers — a void wrapper propagates them
with byte-identical codegen.

## Codegen

EGC reproduces the original's interleaving exactly: the `li a0,CMD` /
`li a1,count` setup instructions are emitted BETWEEN the `buf[i] = arg`
stores, in source-statement order, with the last store (a5) pulled into the
jal delay slot. Frames: 0x20 (4-int buf @0(sp), ra @0x10) and 0x30 (6-int buf
@0(sp), ra @0x20, 0x18-0x1f padding). data = `move a2, sp` (buffer base).
The `int buf[N]` local (not a pointer arg, not a struct) is what places the
store at 0(sp) with &sp as the data pointer.

## Verification

- Objects: 10/10 and 15/15 words identical; R_MIPS_26 on the .text section
  symbol resolves to 0x12E548.
- Full `make` + `cmp`: identical. Count 799 -> 797. decomp-verifier MATCH
  for both.
