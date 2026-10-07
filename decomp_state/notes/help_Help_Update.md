# Help_Update (0x1FDE90, 0xA04 = 641 words) — BLOCKED

In-game help-message state machine: a 9-way `switch (g_helpState.state)` on
`g_helpState` (0x1996D8), each case driving a shared "moby anim" table block plus
case-specific state transitions. INCLUDE_ASM at `code/game/help.cpp:232`.

## Structure (fully decoded from `assets/boot_elf.elf` objdump)

Prologue (MATCHES byte-for-byte): `addiu sp,sp,-0x50` (80-byte frame); saves
`sq s2,32 / ra,64 / s3,48 / s1,16 / s0,0`; `lui v1,0x1a` (hi page) with
`addiu a0,v1,-0x6928` (g_helpState = 0x1996D8); `move s2,v1` puts the 0x1A0000
hi base into s2 (callee-saved). `g_helpState` is therefore always materialized in
the body as `addiu X, s2, -0x6928` where X is a scratch register.

Gate: fade sub-state (fadeState +0x30, fadeTimer +0x34), `inputFlag0 (0x13CAE0)
& 0xF000`, `func_001F96F8(0x78)`; `if (GameMode != 0 || fadeState == 0)
{ state=0; counter=0; field_0x24=-1; return; }`; `counter++; switch(state)`.

Jump table (vram 0x1E7A40, 9 entries + 3 pad): 0→0x1fe0b0, 1→0x1fe0e8, 2→0x1fe258,
3→0x1fe3ac, 4→0x1fe4b8, 5→0x1fe62c, 6→0x1fe794, 7→0x1fe7d8, 8→0x1fdf78.

Epilogue: break entry 0x1fe878 (lq ra), return entry 0x1fe87c (lq s3), return-
variant 0x1fe880 (lq s2, incoming delay reloads s3/ra); then lq s1/s0, `jr ra`,
`addiu sp,sp,0x50`.

Shared tails: 0x1fe77c (state in v1, `or v0,v0,a1; sw zero,4(s3) (counter=0);
or v0,v0,a2; b epilogue; sw v0,0(a0)` — case2/3 state=7); 0x1fe7c8 (state in v0,
counter=0 — case4/7 v0=5, case6 fall-through v0=7).

Moby block (shared shape, cases 1,2,3,4,5,8): idx = g_helpState.field_0x28;
`if (0xFFFEu < D_00141968[idx].id) {} else id++` (unsigned lhu+sltu guard);
`q1 = func_001F96F8(helpAnimTime 0x15EEA4) / 600`; `if (D_00141968[idx].timer < q1)
timer = func_001F96F8(PHASE2_ARG) / 600`; then `bits |= (1u << currentLevelId)
| 0x80000000u` and a per-case tail.

## THE KEY TECHNIQUE (reproduces exact size) — fully-inline subscript

The original loads the moby TABLE BASE 0x141968 into `$s1` ONCE and RECOMPUTES
the entry (`$s1 + field_0x28*8`) before EACH access, RELOADING `field_0x28` from
memory per access (`lw v1,40(s3)` repeated). The ONLY C form that reproduces this
is the fully-inline subscript with NO local index and NO pointer:

    D_00141968[g_helpState.field_0x28].id      // index reloaded from memory each access
    D_00141968[g_helpState.field_0x28].timer
    D_00141968[g_helpState.field_0x28].bits

Case 8 keeps the distinct `helpMobyIdEntries` alias (same address 0x141968) for its
nested `id++` (the id++ block is NESTED inside `if (field_0x20 >= 0)` after the
`field_0x3A == 0` sub-block; `bltz field_0x20<0` jumps straight to the phase1 call
skipping id++).

Local-index / pointer forms let EGC keep the ENTRY pointer in `$s1` across the
phase1 jal (compact) and come out TOO SMALL: `struct MobyAnimEntry* entry=&D_00141968[idx];
entry->...` = 0x92c / 459 diffs; `int idx=...; D_00141968[idx]` = 0x96c / 630 diffs
(prologue regressed to save s4/s5, 0x70 frame); `struct MobyAnimEntry* base=D_00141968;
base[idx]` = 0x92c / 459 (= entry form). ONLY the inline reload form hits exact
0xa04. GENERAL RULE: when the original reloads a struct index from memory per access
(base kept in a callee-saved reg, entry recomputed), use the fully-inline
`arr[global.field]` subscript and do NOT cache the index or a pointer in a local.

## Case semantics (verified against objdump; C matches)

- case 0: `if (field_0x24 < 0) return; idx = Help_FindIndex(field_0x24);
  field_0x20 = idx; if (idx < 0) break; field_0x24 = -1;  // UNCONDITIONAL, in bltz delay slot
  Help_DisplayMessage(); return;`
- case 1: `func_002151D8(5,0);` track block; `if (in4 & 0x10){ moby; counter = 8-counter;
  state = 7; bits; break; } if (counter < 6) return; state = 2; counter = 0; break;`
- case 2: `func_002151D8(5,0);` `if (in4 & 0x10){ moby; state = 7; counter = 0; bits; break; }`
  `r = func_001F96F8(24); if (counter < r) return;`
  `if (mt.field_0x5A != 3){ track = HelpMsgs[field_0x20].field_0x08;
  if (track != -1 && track == mt.field_0x54 - 0x7530) return; }`
  `state = 3; counter = 0; break;`
- case 3: `func_002151D8(5,0);` `if (in4 & 0x10){ moby; state = 7; counter = 0; bits; break; }`
  `if (counter < 8) return; state = 4; counter = 0; break;`
- case 4: `func_002151D8(5,0);` `if (in4 & 0x10){ moby; state = 6; counter = 4-counter; bits; break; }`
  `if (counter < 4) return;` track block (func_00215B10); `state = 5; counter = 0; break;`
- case 5: `func_002151D8(5,0); r = func_001F96F8(420);`
  `in4 = counter < r || (track != -1 && track == mt.field_0x54 - 0x7530 &&
  (mt.field_0x50 != 0 || D_0014001C != -1));`  // D_0014001C is a SEPARATE global at 0x14001C
  `if (in4 && !(g_helpState.field_0x38 & 0x10)) return; moby; state = 6; counter = 0; bits; break;`
- case 6: `if (helpAnimByte (0x15EE1D) && counter < 4 && !(in4 & 0x10)) return; state = 7; counter = 0; break;`
- case 7: `func_002151D8(5,0);` track block: `if (track != -1 && track == mt.field_0x54 - 0x7530
  && (unsigned int)(g_helpState.field_0x5A - 6) >= 2) g_helpState.field_0x5A = 5;`  // sltiu 2 = field_0x5A is 6 or 7
  `if (counter < 8) return; if (g_helpState.field_0x38 != 0) field_0x24 = 8; else { state = 0; field_0x20 = -1; }`
  `counter = 0; break;`
- case 8: `if (field_0x38 != 0) return; if (field_0x20 >= 0){ if (field_0x3A == 0){
  if (FastDecTimer(&field_0x3C) == 0) return; Help_DisplayMessage(); field_0x3A++; break; } }`
  moby (id++ nested); `field_0x20 = -1; state = 0; field_0x3A = 0; bits; break;`

Struct facts: g_helpState (0x1996D8) fields used: fadeState +0x30 (int), fadeTimer
+0x34 (int), field_0x38 (s16), field_0x3A (u16), field_0x3C (int, FastDecTimer arg),
field_0x28 (int idx), field_0x24 (int), field_0x20 (int), counter +0x04 (int),
state +0x00 (int), **field_0x5A (u16)** — the u16 at +0x5A lives on g_helpState,
DISTINCT from musicTransition.field_0x5A. musicTransition (0x1516D0) fields:
field_0x1C@+0x1C (int), field_0x50@+0x50 (int), field_0x54@+0x54 (s16), field_0x5A@+0x5A (s16)
(real offsets — an earlier struct had them at +0/34/38/42, which was wrong).
MobyAnimEntry @ 0x141968: `{ s16 id; u16 timer; u32 bits; }` 8 bytes (id is u16 for
the unsigned 0xFFFE sltu guard).

## THE WALL: 127 fine EGC 2.95.2 body RA/scheduling tie-breaks at EXACT size

Best candidate = 0xa04 (EXACT original size), 127 diff words, match=False.
Prologue, structure, jump table, epilogue, all case semantics, and the moby-table
base-reload all match. The 127 residual diffs are all one class:

1. **g_helpState base register**: original hoists `addiu s0/s3, s2, -0x6928`
   BEFORE the phase1/2 jal (callee-saved, survives the call); the candidate
   recomputes into `a0/a1/a2` AFTER the jal (caller-saved). e.g. case5 0x1fe638:
   orig `addiu s0,s2,..` then `jal`; cand `jal` then `addiu a0,s2,..`.
2. **case-7 track register**: orig `bnel a0,v0` (likely, a0=track); cand `bne a1,v0`
   (plain, a1=track).
3. **branch-likely**: orig `bnezl`/`bnel` where the candidate emits plain `bnez`/`bne`.
4. **delay-slot base reuse**: orig reuses the `lui v1,0x14` delay-slot value for BOTH
   D_0014001C (`lw v0,28(v1)`) and D_0013CAE4 (`lw v0,-13596(v1)`); cand emits a FRESH
   `lui v0,0x14` for D_0014001C (+4 bytes), shifting the case-5 moby block +4 and
   cascading (compensated -4 elsewhere so total size still matches).

These are compiler RA/scheduler tie-breaks, not controllable from the C statement
structure tried.

## Attempts

- Fully-inline subscript form (the exact-size win above) — best result, 127 diffs.
- 4 other moby-block C forms (entry pointer, int idx subscript, base pointer, alias-mix)
  — all 152-216 bytes too small OR 40 bytes too big.
- All case-semantics corrections (musicTransition offsets, g_helpState.field_0x5A u16,
  D_0014001C separate global, case 0/2/5/7 fixes) — applied.
- `--flags=-fno-schedule-insns` (0xa08, 466 diffs) and `-fno-schedule-insns2`
  (0xa10, 641 diffs) — both WORSE; the original used the DEFAULT scheduler.
- No section-attr / prototype / pin lever reproduces the original's per-block
  callee-saved-base / branch-likely / delay-slot-reuse schedule.

## Last-resort escalation

`last-resort-decompiler` (GPT-5.6 Sol) invoked once for this exact target with the
full dossier (exact-size 0xa04/127-diff candidate, original+candidate objdump of the
case-2/5/7 regions, all tried C forms, flag results). It correctly diagnosed the
earlier +40-byte gap as the `helpMobyIdEntries` alias over-use (cases 1-5 should use
D_00141968 for all accesses so the base CSEs into $s1) plus the wrong musicTransition
offsets plus the case 0/5/7 semantic fixes plus explicitly-sequenced ORs — all
corrected (two of its claims were wrong: case-7 field_0x5A is on g_helpState not
musicTransition; case-5 uses D_0014001C not musicTransition.field_0x1C) and applied.
It did NOT anticipate the fully-inline-reload requirement for exact size (found by
local experiment). Its residual-diff lever (isolated -fno-schedule-insns) was tested:
both scheduler flags made size AND diff count worse, confirming the residual 127
tie-breaks have no strictly-C lever in this EGC build. Reverted to INCLUDE_ASM;
full boot-ELF parity green.

## Globals referenced (for a future attempt)

g_helpState 0x1996D8 (in-window). musicTransition 0x1516D0. D_00141968 (moby table,
also aliased as helpMobyIdEntries). D_0014001C (separate global). inputFlag0 0x13CAE0,
inputFlag4 0x13CAE4 (D_0013CAE0/4). helpAnimTime 0x15EEA4, helpAnimByte 0x15EE1D.
GameMode 0x15F604, currentLevelId 0x15ED84. HelpMsgs 0x15F6A0 (stride 0x10,
.field_0x08 = track, -1 = none). magic 0x7530 = track-space offset. callees:
func_001F96F8 0x1F96F8 (timing), FastDecTimer__FRi 0x1F9740, func_002151D8 0x2151D8,
func_00215B10 0x215B10, func_001FDD58 0x1FDD58 (Help_DisplayMessage),
Help_FindIndex 0x1FDCA0. Probe also needs `--define D_0015EEA4=0x15eea4` and
`--define D_0014001C=0x14001c`.
