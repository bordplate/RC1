# func_001EC868 (0x1EC868, 56 bytes) — BLOCKED (EGC DImode constant-base folding)

`code/game/camera.cpp:39`. Suggested name **`Camera_RestorePose`** (mirror of
`Camera_SavePose` @ 0x1EC7F0). It is the restore half of a camera-transition
save/restore pair run from the per-frame caller `func_001EC8A0`.

## Semantics (Ghidra-confirmed; single caller `jal` @ 0x1ECA2C, no args, void)

```c
u8 *base = (u8 *)0x1871B0;            // global cam-transition-state struct (.data, out of gp window)
if (base[2]) {                        // u8 mode byte @ +2
    *(u64 *)(base + 0x50) = *(u64 *)(base + 0xC0);   // active pose A <- slot pose A
    *(u64 *)(base + 0x60) = *(u64 *)(base + 0xD0);   // active pose B <- slot pose B
}
```

Two independent 8-byte (lq/sq) copies. Source words 16 bytes apart (0xC0,0xD0);
dest words 16 bytes apart (0x50,0x60) — **not** a contiguous 16-byte copy.

## Original assembly (objdump ground truth)

```
1ec868: lui   v0,0x18
1ec86c: addiu a2,v0,0x71b0     # a2 = base (SINGLE hoisted base)
1ec870: lbu   v1,2(a2)
1ec874: beqz  v1,0x1ec898
1ec878: addiu a0,a2,0x50       # materialized offset, dst-first (beqz delay slot)
1ec87c: addiu v1,a2,0xC0       # src
1ec880: lq    v0,0(v1)         # REGISTER-based lq/sq
1ec884: sq    v0,0(a0)
1ec888: addiu a1,a2,0xD0       # src-first for pair 2
1ec88c: addiu v1,a2,0x60       # dst
1ec890: lq    v0,0(a1)
1ec894: sq    v0,0(v1)
1ec898: jr    ra
1ec89c: nop
```

## The blocker: DImode constant-base folding gap

Local EGC 2.95.2 **folds** 8-byte constant-base data accesses into offset
addressing (`ld/sd reg, imm(base)`), hoisting the base once. It does NOT emit
the original's `addiu tmp, base, imm; lq/sq reg, 0(tmp)` (fully materialized
register address). Every C form and flag tested reproduces only the folded 44-byte
form (base in a0, `ld/sd off(base)`); the closest is 56 bytes via per-access
`lui` (constant base) but with a different word layout. 13/14 words differ.

This is systematic, not form-dependent:
- The ORIGINAL folds ≤32-bit offsets (`lbu v1,2(a2)`) but NEVER folds 64-bit
  (DImode) constant-base data accesses — it always materializes the address.
  Corpus scan of the boot ELF: **313** materialized `lq/sq 0(reg)` 8-byte DATA
  accesses; zero genuine constant-base folded DImode sites.
- Every camera sibling uses the same idiom (0x1EC8A0, 0x1EC710, 0x1EC7F0,
  0x1ED470, 0x1EDAA8), even with parameter bases.
- **No matched function** (171 matchings) ever exercises 8-byte DATA access at a
  constant base (their only data `sd` is on a runtime pointer; all other lq/sq
  are `$ra`/saved-reg stack ops). This codegen gap has therefore never been hit
  by a match and will govern the other remaining camera.cpp INCLUDE_ASMs.

## Tried (18 probes via tools/decomp_probe.py; all non-matching)

| form | flags | result |
|------|-------|--------|
| const-cast `u8*` base, `*(u64*)(base+off)=...` | default | 44B folded `ld/sd off(base)` |
| same | `-mno-split-addresses` | 44B, per-access lui |
| `extern u8[]` symbol base | default / NOSPLIT | 44B folded |
| signed `s8*` base | default | 44B folded |
| 4 pointer-locals per copy | default / `-fforce-addr` | 56B, per-access lui + folded ld (11/14 words) |
| padded struct field access | default / NOSPLIT / -O1 | 44B folded |
| `u64[]` stride `base[16]` | default | 44B folded |
| `-O1` | several | 44B folded |
| `u128` const base (last-resort form) | default | 56B, per-access lui (11/14 words) |
| `u128` symbol base (last-resort form) | default | 44B folded |

Flags considered in this cc1: `-mno-split-addresses`, `-fforce-addr`,
`-fno-schedule-insns`, `-fno-schedule-insns2`, `-O1/-O2`. None disable DImode
offset folding. (`-fforce-addr` — "copy memory address constants into regs" —
has no effect on the DImode output in any form.)

## Escalation

`last-resort-decompiler` (GPT-5.6 Sol) invoked 2026-09-10. It recommended
reinterpreting the copies as 128-bit (`u128`) — based on reading `lq`/`sq`
(opcodes 0x1E/0x1F) as 128-bit; on the R5900 they are 64-bit quadword ops, and
the +16 word stride (0xC0→0xD0) rules out a contiguous 16-byte copy. Both
`u128` forms it gave were tested mechanically: const base = 56B per-access lui
(11/14 words), symbol base = 44B folded. Neither matches. Recorded as blocked.

## Scope note

`code/game/camera.cpp` is all `INCLUDE_ASM` except `BackupCurrentCam`. The
remaining ~19 camera targets that read this cam-transition-state struct (0x1871B0)
with 8-byte constant-base accesses are subject to the same DImode folding gap and
should be blocked by reference to this note rather than re-investigated
individually. A match requires the original build's flag set (one that disables
DImode split/folding — no such option exists in this cc1) or a future EGC variant.
