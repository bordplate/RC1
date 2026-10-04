# audioDecStart (0x23ACB8, 88 bytes)

Matched 2026-10-04. `extern "C" void audioDecStart(_AudioDec* self)` — part of
the original unmangled movie-decoder callback API (same C-linkage family as
`audioDecSend`/`audioDecIsPageFull`), so the source definition must be
`extern "C"` to emit the unmangled `audioDecStart` symbol.

## Semantics
Starts the movie sound stream. Reads the IOP buffer size (`field_0x4C`), rounds
it down to a 1024-byte boundary with a negative-size guard, issues the start
command, and marks the decoder active (`field_0x00 = 2`).

The one callee, `snd_StartMovieSound` (989snd_post.c:748, C-linkage, matched),
has signature `(int iopBuffer, int iopBufferSize, int iopPausePosition,
int sr, int ch)`. So the argument map is:
- a0 = `field_0x48` (IOP buffer address)
- a1 = rounded `field_0x4C` (IOP buffer size)
- a2 = `field_0x5C` (pause position)
- a3 = `field_0x14` (sample rate)
- a4 = `field_0x18` (channel count)

The single caller is the movie playback loop (FUN_0023a460, 0x23A460) which
calls `audioDecStart(movieDecodeBuf + MOVIE_AUDIO_DEC_OFFSET)` and ignores the
return; the decoder state lives in the movie decode buffer at `0xD9100`.

## Rounding idiom
`v = (size > -1) ? size : size + 0x3FF;` then `(v >> 10) << 10`.
- `size > -1` compiles to `slt t, -1, size` (EGC lowers the literal `-1 < x`
  form to an `slt` with the negative immediate in the source register).
- The ternary lowers to a `movn` (conditional move), not a branch — the source
  must be a ternary expression, not an `if`.
- `(v >> 10) << 10` is a signed (`sra`) then logical (`sll`) pair = truncate to
  a multiple of 0x400 (1024). Do NOT rewrite as `v & ~0x3FF` (that is an `and`
  and does not match).

## Matched form
```c
extern "C" void audioDecStart(_AudioDec* self) {
    int size = self->field_0x4C;
    int aligned = (size > -1) ? size : size + 0x3FF;
    snd_StartMovieSound(self->field_0x48, (aligned >> 10) << 10, self->field_0x5C,
                        self->field_0x14, self->field_0x18);
    self->field_0x00 = 2;
}
```
First probe attempt matched byte-for-byte (decomp_probe 88/88); no register pins
or flags needed. audiodec.o uses default flags / SN assembler.

## Verification
- decomp_probe (working copy): 0 differences, 88/88 bytes.
- tools/fdiff.py 0x23ACB8 0x58: 0 word diffs of 22.
- `make clean && make split && make -j2` then `cmp build/boot_elf.elf
  assets/boot_elf.elf`: byte-identical.
- decomp_status --count: 582 -> 581.

## Style debt (see refactor.json)
- `field_0x00 = 2` is a decoder state with no named constant (sibling
  `audioDecReset` already uses the literal `0`). Name the `_AudioDec` state
  values once the state machine is confirmed.
- `0x3FF` / `10` are the 1024-byte IOP block alignment constants; the `>>10<<10`
  form is codegen-locked, so only the block size could be named.
