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
`v = (size > -1) ? size : size + (AUDIODEC_IOP_BLOCK_SIZE - 1);` then
`(v >> AUDIODEC_IOP_BLOCK_BITS) << AUDIODEC_IOP_BLOCK_BITS`.
- `size > -1` compiles to `slt t, -1, size` (EGC lowers the literal `-1 < x`
  form to an `slt` with the negative immediate in the source register).
- The ternary lowers to a `movn` (conditional move), not a branch — the source
  must be a ternary expression, not an `if`.
- `(v >> 10) << 10` is a signed (`sra`) then logical (`sll`) pair = truncate to
  a multiple of 0x400 (1024). Do NOT rewrite as `v & ~0x3FF` (that is an `and`
  and does not match).
- Division form probe-rejected (2026-10-04): `(v / 0x400) * 0x400` does NOT
  emit a bare sra/sll — EGC applies the signed-division-by-power-of-two fixup
  and emits an extra `slt/addu/movn` correction triplet around the sra
  (candidate 100 bytes vs 88, register allocation shifted as well). Only the
  shift-operator form with a constant count reproduces the original; the log2
  of the block size is therefore named (`AUDIODEC_IOP_BLOCK_BITS`) next to
  `AUDIODEC_IOP_BLOCK_SIZE`.

## State machine (2026-10-04)
The `_AudioDec.state` field (renamed from `field_0x00`; the Deadlocked
descendant names it `state`) has four values, verified from the boot ELF state
tests and `reference/dl/game_dl/movie/audiodec.cpp`:
- `AUDIODEC_STATE_IDLE` (0): written by audioDecCreate and audioDecReset;
  audioDecBeginPut takes the first-put path and audioDecEndPut the
  header-accumulation path for state == 0; audioDecSend does not send.
- `AUDIODEC_STATE_BUFFERED` (1): written by audioDecEndPut once the stream is
  fully buffered; sendADPCM waits for 0x1000 buffered bytes, then sends.
- `AUDIODEC_STATE_STARTED` (2): written by audioDecStart; sendADPCM tracks the
  SPU position via snd_GetMovieNAX.
- `AUDIODEC_STATE_DONE` (3): the send paths return without sending; the write
  site is not in the visible game code (unconfirmed).
Values 0/1/2/3 are consumed by the still-`INCLUDE_ASM` siblings
(audioDecBeginPut, audioDecEndPut, sendADPCM), which keep raw immediates.

## Matched form
```c
extern "C" void audioDecStart(_AudioDec* self) {
    int size = self->field_0x4C;
    int aligned = (size > -1) ? size : size + (AUDIODEC_IOP_BLOCK_SIZE - 1);
    snd_StartMovieSound(self->field_0x48,
                        (aligned >> AUDIODEC_IOP_BLOCK_BITS) << AUDIODEC_IOP_BLOCK_BITS,
                        self->field_0x5C, self->field_0x14, self->field_0x18);
    self->state = AUDIODEC_STATE_STARTED;
}
```
First probe attempt matched byte-for-byte (decomp_probe 88/88); no register pins
or flags needed. audiodec.o uses default flags / SN assembler. The 2026-10-04
style refactor (named state constants + block size, `state` field rename) kept
the same shape and re-verified byte-for-byte.

## Verification
- decomp_probe (working copy): 0 differences, 88/88 bytes.
- tools/fdiff.py 0x23ACB8 0x58: 0 word diffs of 22.
- `make clean && make split && make -j2` then `cmp build/boot_elf.elf
  assets/boot_elf.elf`: byte-identical.
- decomp_status --count: 582 -> 581.
- 2026-10-04 refactor: decomp_probe 88/88 (this function) and 68/68
  (audioDecReset); fdiff 0x23ACB8 0x58: 0/22 and fdiff 0x23AD10 0x44: 0/17;
  clean full build + cmp byte-identical.

## Style debt (resolved 2026-10-04)
- State constants named (AUDIODEC_STATE_* in audiodec.h); both functions use
  them. Block size named (AUDIODEC_IOP_BLOCK_BITS/SIZE); the `>>10<<10`
  form is codegen-locked to the bare sra/sll pair (division rejected, see
  Rounding idiom). Refactor entry cleared.
