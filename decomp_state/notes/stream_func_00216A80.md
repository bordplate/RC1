# func_00216A80 (code/game/stream.cpp) — matched 2026-09-15

## Semantics

`stream_setTransitionVagBufferState`: a VAG track-buffer state callback for a
music transition, sibling of `stream_setVagBufferState` (0x216B28) and the
unmatched 0x216A20 / 0x216AD0 / 0x216B68. The sound system reports each track
buffer's state; the callback records it and advances the buffer's flag.

```cpp
void stream_setTransitionVagBufferState(int state, long buffer) {
    int p = (int)buffer;
    if (p) {
        *(int*)p = state;
        if (state) {
            s16 old = *(s16*)(p + 0xA);
            if (old == VAG_BUFFER_FLAG_QUEUED) {
                *(s16*)(p + 0xA) = 4;
                if (*(s16*)(p + 0x10)) {
                    musicTransition.field_0x20 = old;
                }
            }
        } else {
            *(s16*)(p + 0xA) = 0;
        }
    }
}
```

- Buffer layout (same struct as the sibling VAG callbacks): int state at +0x0,
  s16 flag at +0xA, s16 at +0x10. The flag values mark which track path owns the
  buffer: 2 preseek (0x216A20), 4 transition/start-body (here and 0x216AD0), 8
  start-track (0x216B28), 7 update (0x216B68).
- A nonzero state on a queued (1) buffer promotes the flag to 4. A cleared state
  resets the flag to 0.
- `musicTransition.field_0x20` is D_001516F0 (musicTransition + 0x20, a new
  `s16` field added to `MusicTransState` by splitting the former
  `pad_0x0B[0x49]`). It is raised to `old` (=1) only when the buffer also
  carries a nonzero value at +0x10.

## Codegen notes (why the C shape is what it is)

- THE GOTCHA: the flag store `*(s16*)(p + 0xA) = 4` is UNCONDITIONAL once
  `old == VAG_BUFFER_FLAG_QUEUED`; it is NOT gated on the +0x10 value. In the
  original the `sh v0, 0xA(a1)` sits in the DELAY SLOT of `beqz v1, out`, and
  `beqz` is an ordinary (non-likely) branch, so that store executes on BOTH
  outcomes of the +0x10 test. Only the global store `musicTransition.field_0x20
  = old` (in the `jr $ra` delay slot) is gated on +0x10. Nesting the flag store
  inside `if (*(s16*)(p + 0x10))` makes EGC keep it after the `beqz`, hoists
  `lh v1, 0x10(a1)` into the `bne` delay slot instead of `li v0,4`, loads the
  global base into v1 before the flag store, and comes out 4 bytes too long —
  this misread cost many probe variants before the delay-slot reading resolved
  it.
- `*(int*)p = state` sits OUTSIDE `if (state)` (scheduled into the `beqz state`
  delay slot, executed on both paths), matching the sibling.
- The global store is written through `musicTransition.field_0x20` (a named
  struct field at +0x20), which codegens to `lui v0, %hi(musicTransition);
  sh a0, %lo(musicTransition)+0x20(v0)` — byte-identical to the reference's
  `%hi/%lo(D_001516F0)` (both resolve to 0x1516F0). A plain
  `extern "C" s16 D_001516F0 __attribute__((section(".data")))` also matches;
  the struct-field form is preferred to avoid an address-based name.
- No frame: the function makes no calls, so EGC emits no prologue.

## Original body (vram 0x216A80, 0x4C bytes)

```
dsll32 a1, a1, 0
dsra32 a1, a1, 0
beqz   a1, out
    nop
beqz   a0, else
    sw   a0, 0(a1)          # *(int*)p = state (both paths)
lh     a0, 0xA(a1)          # old
li     v0, 1
bne    a0, v0, out
    li   v0, 4              # bne delay slot
lh     v1, 0x10(a1)         # +0x10
beqz   v1, out
    sh   v0, 0xA(a1)        # flag = 4 (beqz delay slot -> UNCONDITIONAL)
lui    v0, %hi(D_001516F0)  # base (reuses v0 after the sh)
jr     ra
    sh   a0, %lo(D_001516F0)(v0)  # field_0x20 = old (jr delay slot, gated)
else:
sh     zero, 0xA(a1)        # flag = 0
out:
jr     ra
    nop
```

## Verification

- Probe `decomp_probe.py` on the candidate: 76/76 bytes, match true.
- `make clean && make split && make -j2`: clean build.
- `cmp build/boot_elf.elf assets/boot_elf.elf`: identical.
- `tools/decomp_status.py --count`: 689 -> 688.
