# SetMobyLights (0x20D4F0)

Matched 2026-10-09. Replaced the `INCLUDE_ASM` for the Splat region
`func_0020D4E0` (0x30 bytes) in `code/game/mobyfunc.cpp`.

## Region layout

The 0x30-byte region `func_0020D4E0` is two parts:

- `0x20D4E0` (0x10 bytes): **dead tail** — `addiu sp,sp,0x70` + 3 nops.
- `0x20D4F0` (0x20 bytes): the real function (alabel `func_0020D4F0`).

## Semantics

`u64 SetMobyLights(MobyInstance* m, u64 high, u64 lo0, u64 lo1, u64 lo2)`:

```
m->unk1 = (high << 0x20) | lo0 | (lo1 << 8) | (lo2 << 0x10);
return m->unk1;
```

`m->unk1` is the u64 at `MobyInstance` offset 0x38 (32-bit-pointer layout:
`bSphere` 0x00, `pos` 0x10, `state/group/mClass/alpha` 0x20-0x23,
`pClass` 0x24, `pChain` 0x28, `scale` 0x2C, `updateDistance` 0x30,
`enabled` 0x31, `drawDistance` 0x32, `modeBits` 0x34, `modeBits2` 0x36,
`unk1` 0x38). The packed value is the moby's lights word.

Named from the Deadlocked reference:
`reference/dl/game_dl/mobyfunc.cpp:2058` calls
`SetMobyLights(pmVar2,0x202020,0xe,0xe,0);` and
`reference/dl/FUNCTIONS.txt:151687` gives the body. The only boot-ELF caller is
`jal` at 0x2254FC (func_00225490, pause_post3, still INCLUDE_ASM) passing
`(moby, 0x202020, 0xE, 0xE, 0)` and ignoring the return.

## Match-sensitive codegen

The original 8 words:

```
0005283c  dsll32 a1,a1,0      ; high shift IN-PLACE in a1
00073a38  dsll   a3,a3,8
00084438  dsll   t0,t0,16
00a61025  or     v0,a1,a2     ; v0 accumulator, reads a1 (not v0)
00471025  or     v0,v0,a3
00481025  or     v0,v0,t0
03e00008  jr     ra
fc820038  sd     v0,56(a0)
```

Three things had to be reproduced, none of which the naive C form gives:

1. **Returns u64, not void.** A `void` body (naive `m->unk1 = ...`) accumulates
   the or-chain in `a1` and stores `sd a1`. The original keeps the result in
   `v0` (the return register), so the function must `return` the packed value.

2. **High shift stays in-place in `a1`.** With a u64 return, EGC folds
   `high << 0x20` into the `v0` accumulator (`dsll32 v0,a1,0; or v0,v0,a2`).
   Forcing `register u64 highS asm("$5") = high;` pins the high field to `$5`
   (a1), so the shift is emitted in-place (`dsll32 a1,a1,0`) and the first `or`
   reads a1 (`or v0,a1,a2`).

3. **Shifts-first, non-interleaved schedule.** Without barriers EGC schedules
   the first `or` before the last shift (`or v0,...` at offset 0x8, the
   `lo2 << 0x10` shift at 0xc). Three zero-byte tied barriers
   `asm volatile("" : "+r"(x))` after each shift force all three shifts to
   complete (in source order high, lo1, lo2) before the first `or`.

The final matching body:

```cpp
u64 SetMobyLights(MobyInstance* m, u64 high, u64 lo0, u64 lo1, u64 lo2) {
    register u64 highS asm("$5") = high;
    highS <<= 0x20;
    asm volatile("" : "+r"(highS));
    lo1 <<= 8;
    asm volatile("" : "+r"(lo1));
    lo2 <<= 0x10;
    asm volatile("" : "+r"(lo2));
    u64 lights = highS | lo0 | lo1 | lo2;
    m->unk1 = lights;
    return lights;
}
```

## Dead tail

The 0x10-byte tail at 0x20D4E0 is a ghost: `deadness_scan.py 0x20D4E0` reports
0 references and no nearby function uses a 0x70 frame (DrawMobys at 0x20D460 is
frameless). It is the EGC "dead `addiu sp,sp,N` after the parent's RTL"
artifact (see the 989snd family). Preserved as exact words in a
`.section .text` block with `nonmatching func_0020D4E0, 0x10` /
`glabel`/`endlabel`, following the `code/game/actuator.cpp` precedent.

## Symbol

`SetMobyLights__FP12MobyInstanceUlUlUlUl = 0x0020D4F0;` added to
`config/symbols.txt`; `make split` regenerated the caller's `.s` to
`jal SetMobyLights__FP12MobyInstanceUlUlUlUl`. The `.ld` places
`mobyfunc.o(.text)` as a contiguous block, so the function lands at 0x20D4F0 by
source order (dead tail 0x10, then the 0x20 function) with no per-symbol pin.

## Verification

- Per-function: objdump of `build/boot_elf.elf` at 0x20D4F0 == original (8 words).
- Full: `cmp build/boot_elf.elf assets/boot_elf.elf` passes (BUILD-PARITY-OK).
- Count: 542 -> 541 nonmatching.
