# music_Pause__Fi (vram 0x216050, file 0x116FD0, 52 bytes)

Semantics: pauses the SIF music driver channels. Writes the three
(state, value) s16 pairs of `MusicState` at D_001516D0 — (0x40, 0x42),
(0x78, 0x7A) unconditionally, and (0x5C, 0x5E) additionally when the int
parameter is nonzero — to (-0x8000, 0). 0x8000 is the "paused" state code;
sibling `music_Unpause__Fv` (0x216088) writes 4 to the state halfwords.
All three callers (0x1FBAE8, 0x218D90, 0x230490) pass 0, so the guarded
pair is currently dead but present.

## Original body (13 words)

```
beqz   a0, +0x18
  lui  a1, %hi(D_001516D0)          (bds; see note below)
addiu  v1, a1, %lo(D_001516D0)
addiu  v0, $0, -0x8000
sh     v0, 0x5C(v1)
sh     $0, 0x5E(v1)
addiu  v0, a1, %lo(D_001516D0)
addiu  v1, $0, -0x8000
sh     v1, 0x78(v0)
sh     $0, 0x7A(v0)
sh     v1, 0x40(v0)
jr     ra
sh     $0, 0x42(v0)                (bds)
```

## Candidate (committed)

`code/game/music.cpp`:

```cpp
void music_Pause(int param_1) {
    if (param_1 != 0) {
        D_001516D0.field_0x5C = -0x8000;
        D_001516D0.field_0x5E = 0;
    }
    D_001516D0.field_0x40 = -0x8000;
    D_001516D0.field_0x42 = 0;
    D_001516D0.field_0x78 = -0x8000;
    D_001516D0.field_0x7A = 0;
}
```

`MusicState` fields changed u16 -> s16 and extended with field_0x42/0x5E/
0x7A (offsets of 0x40/0x5C/0x78 unchanged, so `music_Unpause__Fv` codegen
is unaffected — re-verified byte-identical after the change).

## Findings

1. **s16, not u16, for 0x8000 halfword constants.** Storing -0x8000 into a
   `u16` makes EGC materialize 32768 with `ori $v0,$0,0x8000` (word
   34028000); into an `s16` it emits `addiu $v0,$0,-0x8000` (word
   24028000), which is what the original has. Single-word diff otherwise.
   (Probe: decomp_state/probes matrix of u16/s16 fields; same value,
   different materialization instruction.)

2. **4 independent constant stores, one %hi/%lo base: pair-swap
   permutation.** Source [s1,s2,s3,s4] emits [s3,s4,s1,jr ra,<bds s2>]:
   consecutive 2-store groups keep their inner order but the two groups
   swap. Source order 40,42,78,7A reproduces the original
   78,7A,40,[jr]42; the naive machine order 78,7A,40,42 instead emitted
   40,42,78,[jr]7A. Consistent with the known 2-store reversal and the
   fixed 3-store permutation (see music_Unpause__Fv.md / AGENTS.md).

3. **EGC schedules the base `lui` into the `beqz` branch delay slot.**
   The hi halfword of D_001516D0 lands in $a1 in the bds of the first
   `beqz`, i.e. it is materialized ONLY on the not-taken path; the
   fall-through block then uses the (uninitialized-on-that-path) $a1.
   Plain direct-global struct access reproduces this exactly — no pointer
   local, no constant-address cast needed. The lo-add is recomputed into
   a different register per block (a2-style v1 in block 1, v0 in block 2).

4. **Definition order in the TU must match original .text order.**
   Original has music_Pause at 0x216050 BEFORE music_Unpause at 0x216088,
   so the C definition of `music_Pause` must precede `music::Unpause()`
   in music.cpp. First attempt defined it after Unpause: object diff was
   clean but full-image parity failed with 61 differing bytes (the two
   functions swapped 0x20 bytes, plus two caller `jal`s at file
   0xFCA6C/0xFE638 resolving to the wrong target). Moving the definition
   before `class music` fixed it.

## Verification

- music.o: `T music_Pause__Fi` 13 words byte-identical to original (fixed
  words compared; R_MIPS_HI16/LO16 on D_001516D0 at +4/+8/+16 resolve to
  0x15/0x16D0). music_Unpause__Fv re-verified unchanged (7 words).
- decomp-verifier: `make clean && make split && make -j2` + `cmp
  build/boot_elf.elf assets/boot_elf.elf` byte-for-byte identical; raw
  words at file 0x116FD0 identical in both ELFs.
- decomp_status --count 755 -> 754.
