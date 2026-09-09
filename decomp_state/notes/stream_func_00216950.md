# func_00216950 (code/game/stream.cpp) — matched 2026-09-09

## Semantics

IOP event callback for the "safe CD" music streaming state. Registered via
`snd_StreamSafeCdCallback(func_00216950)` in `func_00215420` (music.cpp,
already matched); the driver stores the pointer in the gp-relative global at
0x0015ECF0 (gp-0x7F70) and invokes it from `snd_FlushSoundCommands`
(0x12DE0C..0x12DE20): after `snd_StreamSafeCdSync(1)` it tests a second
driver flag (gp-0x7F68), clears both flags, then `jalr callback` with the
delay slot `addiu $4,$0,1` — so the callback is ALWAYS called with
`param_1 = 1` and its return value is never used (the next driver
instruction overloads v0).

Body when `param_1 == 1`: clear the substate word
`D_001516D0.field_0x08` (=0x001516D8) to 0, then query
`snd_StreamSafeCdGetError()`; if there is a CD error set the substate to 2.
So field_0x08 is a 16-bit streaming substate: 1 = streaming (set by the
other callback path), 2 = error, 0 = cleared/pending.

## Original body (file offset 0x1178D0, vram 0x00216950, 15 words)

```
addiu sp,sp,-0x20
li    v0,1
sq    ra,0x10(sp)
bne   a0,v0,.L00216980
    sq  s0,0(sp)               # delay
lui   v0,%hi(D_001516D0)
addiu s0,v0,%lo(D_001516D0)
jal   snd_StreamSafeCdGetError
    sh  zero,0x8(s0)           # delay: field_0x08 = 0
beqz  v0,.L00216980
    li  v0,2                   # delay: executes on BOTH paths
sh    v0,0x8(s0)               # field_0x08 = 2
.L00216980:
lq    ra,0x10(sp)
lq    s0,0(sp)
jr    ra
    addiu sp,sp,0x20
```

## Replacement (stream.cpp)

```cpp
extern "C" int snd_StreamSafeCdGetError(void);

extern "C" void func_00216950(int param_1) {
    if (param_1 != 1) {
        return;
    }
    D_001516D0.field_0x08 = 0;
    int err = snd_StreamSafeCdGetError();
    if (err) {
        D_001516D0.field_0x08 = 2;
    }
}
```

`MusicTransState` in stream.cpp was extended with `s16 field_0x08` (the
struct declaration moved above the function; definition order of the
functions in the TU is unchanged, so .text layout is unaffected). The
caller-side prototypes in music.cpp were updated from `int` to `void`
return (pure declaration change, no codegen impact).

## Codegen findings (EGC 2.95.2, project flags)

1. **Store-before-call decides the register class of the base.** With the
   `field_0x08 = 0` store written AFTER the `GetError()` call, EGC computed
   the global base lazily AFTER the jal into v1 (volatile), emitted a 0x10
   frame with no s0 save — 3-word structural mismatch. Writing the store
   BEFORE the call makes the base live across the call: EGC keeps it in s0,
   0x20 frame (sq ra 0x10, sq s0 0), and schedules the `sh zero,8(s0)` into
   the jal delay slot, exactly as the original.

2. **The function is VOID; v0=2 at exit is a leftover.** An `int`-returning
   version compiled all 15 original words PLUS a 16th: a `li v0,2` reload at
   the branch merge (EGC schedules `li v0,2` into the `beqz` delay slot as
   the store-data load, but the delay-slot fill is a post-pass, so the RTL
   still believed v0=err=0 on the taken path and re-materialized the return
   constant at the merge). With `void` there is no return-value constraint,
   the store-data `li v0,2` lands in the delay slot and is simply left in
   v0 at `jr ra` — the single-load signature from the space_func_0022E188
   observation. The `li v0,1` at the top is the `bne` comparison constant in
   the free v0, not a return value. The sole caller ignores the result, so
   void is also semantically correct.

3. **Early-return form keeps the bne direction.** `if (param_1 != 1)
   return;` compiles to `li v0,1; bne a0,v0,epilogue` with the s0 save in
   the bne delay slot — the original's shape. (An `if (param_1 == 1) { ... }`
   wrap would invert the branch and move the s0 save.)

## Verification (mechanical)

- `build/code/game/stream.o` func_00216950: 15/15 words identical to the
  original; relocations R_MIPS_HI16/LO16 on D_001516D0 (link to
  `lui v0,0x15 / addiu s0,v0,0x16D0`) and R_MIPS_26 on
  snd_StreamSafeCdGetError (links to 0x12EEF0).
- Built ELF slice at file offset 0x1178D0 (0x38 bytes) byte-identical to
  assets/boot_elf.elf.
- `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf` — identical.
- Splat reclassified: func_00216950.s now under
  code/_generated/matchings/game/stream/; status count 737 -> 736.

## Follow-ups

- func_002166E8 (the sibling at 0x2166E8, same struct) tests
  `field_0x08` (lh) and sets `field_0x0A = 1` (sb) after
  snd_StreamSafeCdBreak(); field_0x0A/field_0x08 deserve real names once
  both siblings plus the driver flags (0x15ECF0/0x15ED08) are decoded.
- snd_StreamSafeCdGetError/Break/Callback/Sync/Read (989snd.c) are small
  wrappers over snd_SendIOPCommand* and are natural next targets.
