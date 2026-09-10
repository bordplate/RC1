# func_002166E8 (code/game/stream.cpp) — matched 2026-09-10

## Semantics

Music-transition CD break. If the music transition has a pending CD status,
issue a stream-safe CD break and raise the break-pending flag:

```cpp
void stream_breakTransitionCd(void) {
    if (musicTransition.field_0x08 != 0) {
        snd_StreamSafeCdBreak();
        musicTransition.field_0x0A = 1;
    }
}
```

- `musicTransition` is the global `MusicTransState` at 0x1516D0 (out of the
  gp window, so a plain `extern` gives a signed %hi/%lo split).
- `field_0x08` (s16 at +8): transition CD status, set by
  `stream_updateCdStatus` to 0 (ok) or 2 (CD error, from
  `snd_StreamSafeCdGetError`).
- `field_0x0A` (u8 at +0xA): break-pending flag. Set to 1 here; cleared by
  the transition state machine `FUN_00216290` (0x216290) once it polls
  `0x12EE08(1)` and it returns 0 (break complete), at which point it also
  clears `field_0x08`.

Four callers, all in pause_post, `jal` it by the address-based symbol:
0x21BE1C (FUN_0021bda0), 0x225D3C, 0x21D2F4, 0x226744. They invoke it when a
pause/transition path sees `field_0x08 != 0`.

## Original body (file offset 0x117668, vram 0x2166E8, 0x40 bytes)

```
addiu sp,sp,-0x20
lui   v0, %hi(musicTransition)
sq    s0, 0(sp)
sq    ra, 0x10(sp)
addiu s0, v0, %lo(musicTransition)   # s0 = &musicTransition
lh    v1, 8(s0)                      # v1 = field_0x08
beqz  v1, exit
    lq  ra, 0x10(sp)                 # beqz delay slot
jal   snd_StreamSafeCdBreak
    nop
addiu v1, 0, 1                       # v1 = 1  (reuses the condition register)
sb    v1, 0xA(s0)                    # field_0x0A = 1
lq    ra, 0x10(sp)
exit:
lq    s0, 0(sp)
jr    ra
    addiu sp, sp, 0x20
```

## Replacement (code/game/stream.cpp)

```cpp
typedef struct {
    u8  pad_0x08[0x08];
    s16 field_0x08;
    u8  field_0x0A;
    u8  pad_0x0B[0x49];
    s16 field_0x54;
    s16 field_0x56;
    s16 field_0x58;
} MusicTransState;

extern MusicTransState musicTransition __attribute__((section(".data")));

extern "C" int snd_StreamSafeCdGetError(void);
extern "C" int snd_StreamSafeCdBreak(void);   // int, NOT void — see codegen note

// Symbol override: four pause_post callers jal it by the address-based name.
void stream_breakTransitionCd(void) asm("func_002166E8");

void stream_breakTransitionCd(void) {
    if (musicTransition.field_0x08 != 0) {
        snd_StreamSafeCdBreak();
        musicTransition.field_0x0A = 1;
    }
}
```

The `asm("func_002166E8")` override follows the `draw_resetTextureDmaState`
pattern in code/game/draw.cpp. `field_0x0A` was carved out of the previous
`pad_0x0A[0x4A]` (now `u8 field_0x0A; u8 pad_0x0B[0x49];`).

## Codegen findings (EGC 2.95.2, project flags)

1. **Callee return type picks the post-call constant register.** The constant
   store (`field_0x0A = 1`) sits in a basic block AFTER the `jal`, so it is a
   fresh value the reload must place in a dead temporary. Both `v0` (held the
   base `%hi`, dead after `addiu s0,v0,%lo`) and `v1` (held the condition, dead
   after `beqz`) are free. The original puts the constant in **v1** (reusing the
   condition's register); a `void` declaration of the callee makes EGC put it in
   **v0** (reusing the base-hi register) -> 2-word diff at 0x216710/0x216714
   (`24020001/a202000a` vs the original `24030001/a203000a`).

   `snd_StreamSafeCdBreak` (0x12EEA8) is NOT void: its active path ends in
   `b <epi>` with `li v0,1` in the delay slot, and its inactive path
   `jal func_001216C8; nop` forwards that callee's 32-bit return. Declaring it
   `extern "C" int` keeps `$v0` claimed across the call, so EGC allocates the
   fresh constant to `v1` and the function matches byte-for-byte. This is the
   same verified callee-prototype effect as `snd_SendCurrentBatch`
   (AGENTS.md, 2026-09-06): an ignored return value still affects register
   allocation. Confirm CALLEE return types before blaming allocator tie-breaks.

2. **Double `lq ra` is automatic.** The `lq ra,0x10(sp)` appears both in the
   `beqz` delay slot and after the store; no C trick needed (same as the
   matched sibling func_002169C0).

3. C forms that do NOT change the constant register (all still v0 with a void
   callee): implicit `if (field_0x08)`, a local reused for condition+constant
   (moves the condition to v0 too), a struct pointer, a pointer-cast store, a
   cast constant, a u8 local. Scheduler flags (-fno-schedule-insns /
   -fno-schedule-insns2) don't help (the register is chosen by reload,
   pre-scheduling).

## Verification (mechanical)

- decomp_probe on the final C: 64/64 bytes, 0 differences.
- `build/code/game/stream.o` `func_002166E8`: 16/16 words identical to the
  original (objdump), symbol emitted as `func_002166E8` via the asm override,
  next function `func_00216728` still at +0x40.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf`: identical.
- decomp_status --count: 725 -> 724.

## Last-resort escalation

Before this match, the 2-word v1/v0 diff was diagnosed as an EGC reload
tie-break after ~11 C/flag probes failed. `last-resort-decompiler`
(GPT-5.6 Sol) was invoked and returned the correct, decisive recommendation:
the callee `snd_StreamSafeCdBreak` returns an int, not void. Applying the
`int` prototype produced the match on the next probe.
