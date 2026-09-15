# func_00216B28 (code/game/stream.cpp) — matched 2026-09-15

## Semantics

`stream_setVagBufferState`: the VAG track-buffer state callback that
`music_StartTrack__Fiii` (0x215C40) passes as the stack argument callback to
`snd_PlayVAGStreamByLocEx_CB` (0x12ec08). The sound system invokes it for each
track buffer as the VAG stream progresses, reporting the buffer's state.

```cpp
void stream_setVagBufferState(int state, long buffer) {
    int p = (int)buffer;
    if (p) {
        *(int*)p = state;
        if (state) {
            if (*(s16*)(p + 0xA) == VAG_BUFFER_FLAG_QUEUED) {
                *(s16*)(p + 0xA) = VAG_BUFFER_FLAG_READY;
                return;
            }
        } else {
            *(s16*)(p + 0xA) = 0;
        }
    }
}
```

- Buffer layout (same struct as the sibling VAG callbacks at 0x216A20-0x216C48):
  int state at +0x0, s16 flag at +0xA, s16 at +0x10 (sibling 0x216A80/0x216AD0),
  u32 at +0x18 (sibling 0x216BC0). The flag values across the family mark which
  track path owns the buffer: 2 preseek (0x216A20), 4 transition/start-body
  (0x216A80/0x216AD0), 8 start-track (here), 7 update (0x216B68).
- A nonzero state is recorded on the buffer and promotes a queued (1) buffer to
  ready (8). A cleared state resets the flag to 0.
- The `int p = (int)buffer` idiom and raw `+ 0xA` offset follow the matched
  siblings stream_setBufferState/stream_updateBufferState in this file; only the
  flag VALUES are #defined (preprocessor-only, codegen-identical).

## Codegen notes (why the C shape is what it is)

- `*(int*)p = state` must sit OUTSIDE `if (state)` even though semantically it
  looks conditional: in the original the store is scheduled into the `beqz
  state` delay slot, so it executes on BOTH paths (storing 0 when state is 0).
  Nesting it inside `if (state)` makes EGC slot `li v0,1` into the delay slot
  instead and emits the store after the `bne` — a 2-word diff (the only diff
  found; everything else was byte-identical on the first attempt).
- The `return;` after `*(s16*)(p + 0xA) = 8;` is required: with no explicit
  return the then-branch falls into a forward `b out` jumping over the else
  block (as in matched `stream_setBufferState` 0x2169C0, which has a call in
  its else), but the original ends the then-branch with a direct `jr $ra` whose
  delay slot holds the `sh` — an explicit return is what EGC compiles to that
  direct `jr $ra`.
- No frame: like `stream_updateBufferState` (0x216990), the function makes no
  calls, so EGC emits no prologue; the `beqz p` delay slot is a nop.

## Original body (vram 0x216B28, 0x3C bytes)

```
dsll32 a1, a1, 0            # p = (int)buffer
dsra32 a1, a1, 0
beqz   a1, out
    nop
beqz   a0, else
    sw   a0, 0(a1)          # *(int*)p = state (both paths)
lh     v1, 0xA(a1)
li     v0, 1
bne    v1, v0, out
    li   v0, 8              # bne delay slot
jr     ra                   # explicit return
    sh   v0, 0xA(a1)        # *(s16*) = 8
else:
sh     zero, 0xA(a1)        # *(s16*) = 0
out:
jr     ra
    nop
```

## Verification

- decomp-verifier ran `make clean && make split && make -j2`: clean build.
- `cmp build/boot_elf.elf assets/boot_elf.elf`: identical.
- `objdump` diff of 0x216B28-0x216B64 between build and original: identical.
- Full stream.o .text region (0x2166E8-0x2170C8) diff: identical.
