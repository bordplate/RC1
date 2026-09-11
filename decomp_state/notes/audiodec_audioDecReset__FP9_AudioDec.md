# audioDecReset__FP9_AudioDec (vram 0x23AD10, file 0x13BC90, 68 bytes) — MATCHED 2026-09-11

```
addiu sp,sp,-0x20
sq    s0,0(sp)
sq    ra,0x10(sp)
jal   snd_ResetMovieSound      ; 0x12F0A8
daddu s0,a0,0                  ; delay slot: s0 = self
sw    zero,0x5C(s0)
sw    zero,0x0(s0)
sw    zero,0x30(s0)
sw    zero,0x38(s0)
sw    zero,0x3C(s0)
sw    zero,0x44(s0)
sw    zero,0x50(s0)
sw    zero,0x58(s0)
lq    ra,0x10(sp)
lq    s0,0(sp)
jr    ra
addiu sp,sp,0x20               ; delay slot
```

## Identity / context

Movie-audio reset. Stops the movie sound and zeroes the `_AudioDec` state
fields, preparing the decoder for a fresh movie.

```c
void audioDecReset(_AudioDec* self) {
    snd_ResetMovieSound();
    self->field_0x00 = 0;
    self->field_0x30 = 0;
    self->field_0x38 = 0;
    self->field_0x3C = 0;
    self->field_0x44 = 0;
    self->sentPos  = 0;
    self->field_0x58 = 0;
    self->field_0x5C = 0;
}
```

- `snd_ResetMovieSound` (vram 0x12F0A8, 989snd sound library): C-linkage,
  no-arg movie-sound reset. The `jal` delay slot is used for `s0 = a0`, so the
  callee takes no argument (void prototype).
- The `_AudioDec` is embedded in the large movie struct at D_0016120C + 0xD9100
  (same location used by isAudioOK / proceedAudio / audioDecDelete); the reset
  walks the struct fields and clears them.

## Struct layout finding

The previous `_AudioDec` was `{ u8 _pad[0x50]; int sentPos; }` (0x54 bytes).
This function stores to 0x58 and 0x5C, which are beyond that. Scanning the
generated sibling functions for all `self`-based `lw`/`sw` established the full
observed int offsets: 0x00, 0x04, 0x14, 0x18, 0x1C, 0x30, 0x34, 0x38, 0x3C,
0x40, 0x44, 0x48, 0x4C, 0x50, 0x58, 0x5C, 0x60 (every one a 4-byte access; e.g.
audioDecStart loads 0x14, sendADPCM loads/stores 0x60, audioDecCreate stores
0x04/0x34/0x40/0x48/0x4C). The struct was extended to 0x64 with offset-based
field names (`field_0xNN`) and `pad_NN` only for the three genuine unobserved
gaps (0x08-0x13, 0x20-0x2F, 0x54-0x57), per STYLEGUIDE.md. `sentPos` stays at
0x50 and the 0x00 field keeps its position, so the already-matched
`audioDecIsPageFull` (sentPos) and `audioDecSend` (`*(int*)self`) are
unaffected. `_AudioDec` is TU-local to audiodec.cpp and only ever used as a
pointer (never allocated or `sizeof`'d), so the size growth is safe.

## Codegen finding: 8 constant stores right-rotate by 1

For eight independent `sw zero,off(s0)` stores followed by the standard
`lq ra; lq s0; jr ra; <addiu sp>` epilogue (no constant return, nothing in the
`jr` delay slot), EGC 2.95.2 emits the stores in **right-rotation-by-1** of the
source statement order: source `[a,b,c,d,e,f,g,h]` -> machine
`[h,a,b,c,d,e,f,g]`. The original machine order is
`[0x5C,0x00,0x30,0x38,0x3C,0x44,0x50,0x58]`, so the matching source order is the
left-rotation `[0x00,0x30,0x38,0x3C,0x44,0x50,0x58,0x5C]` — which is also the
natural ascending offset order. (Contrast the 2- and 3-store permutations
documented for constant-return tails in AGENTS.md; this call+stores-then-epilogue
shape has its own rule.)

## Verification

- `decomp_probe.py` candidate vs generated reference: 68/68 bytes, 0 diffs.
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf`: byte-for-byte OK.
- Nonmatching count 724 -> 723.
