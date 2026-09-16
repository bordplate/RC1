# SetPauseActionList (0x21A1B0, 44 bytes) — MATCHED

`code/game/pause.cpp` (was `func_0021A1B0`). Renamed via
`config/symbols.txt` (`SetPauseActionList = 0x0021a1b0;`) + rerun `make split`;
Splat then moved the reference to `code/_generated/matchings/game/pause/SetPauseActionList.s`
and updated the data reference at 0x1D4870 (function-pointer table in the core
data blob) to `.word SetPauseActionList`.

```cpp
typedef struct {
    u8 pad[0x34];
    u32 actionList;
} PauseActionMode;

extern int pauseActionListA[];
extern int pauseActionListB[];
extern int pauseActionListMode;

extern "C" int SetPauseActionList(PauseActionMode* mode) {
    // EGC's named -G0 load allocates a separate base register (lui $v1;
    // lw $v0, off($v1)); the original reuses one register (lui $v0;
    // lw $v0, off($v0)). A bare lw pseudo makes ps2eeas expand the pair
    // in place self-based (no .extern precedes the reference).
    int listMode;
    asm volatile("lw %0, pauseActionListMode" : "=r"(listMode));
    if (listMode != 0)
        mode->actionList = (u32)pauseActionListA;
    else
        mode->actionList = (u32)pauseActionListB;
    return 0;
}
```

## Semantics

Selects one of two static action/entry tables for the pause-screen mode struct
(global instance at 0x1D5BF4) based on the boot flag at 0x15EE90:

- `D_001D4810`: 3 entries of 6 shorts —
  {0xF22B,0x0034,0x4940,0x001D,0,0}, {0xF221,0x0034,0xAB8,0x001D,0,0},
  {0xF222,0x0034,0x8B8,0x001D,0,0}
- `D_001D4840`: 2 entries — {0xF22B,0x0034,0x4940,0x001D,0,0},
  {0xF222,0x0034,0x8B8,0x001D,0,0}

The consumer (0x21ABF8, same function-pointer family; the table at 0x1D4868
holds {func_0021ABF8, func_0021B1C8, SetPauseActionList}) walks `mode+0x34`
with a 12-byte stride: field 0 (0xF2xx) is the entry id / loop terminator
(0 ends the list), fields 4-7 are a 32-bit data pointer (0x1D4940/0x1D48B8/
0x1D4AB8 → small `{0,1,0xCA,0xD2}`-style parameter records), and entries are
executed in order (cases 2-0xb: SFX via FUN_0022da68, `mode_freezeInit(3, ptr)`,
state transitions, etc.).

Flag 0x15EE90 is set once in `InitOnce__Fv` (0x201650) as
`(char)buffer+0x33 != 'N'` for a memory-card name/info buffer, and is also
read by `memcard_Update` (0x2093d8) to pick save size 0x3C04 vs 0x3C00.

## Codegen findings (EGC 2.95.2)

1. **Integer literals → branchless movz (wrong shape).** With `val =
   (flag==0) ? 0x1D4840 : 0x1D4810` as int constants, EGC preloads both into
   a5/a6 and selects with `movz` (plus hoisted `move v0,0` and `j ra` with the
   store in the delay slot) — 9-word diff vs the original branch-based layout.
   The values MUST be symbol addresses (`extern int D_001D4810[];` +
   `(int)D_001D4810`) so each branch materializes its own lui/addiu.
2. **Branch polarity inversion.** Written as `if (flag == 0) A; else B;`,
   EGC emits `bne v0,$0` (condition negated, bodies swapped). The original is
   `beq v0,$0` with the ==0 case starting in the delay slot. Writing the
   condition as `if (flag != 0) A; else B;` makes EGC emit the original
   `beq ==0` layout exactly (v2 → v3: 5 diffs → 0).
3. **Flag load base register.** 0x15EE90 is inside the gp window; a plain
   `extern int` symbol load gives `lui v1; lw v0,off(v1)` (base in v1), while
   the original reuses v0 for base and value (`lui v0; lw v0,off(v0)`). The
   constant-address cast `*(int *)0x15EE90` also reproduced the original, but
   is forbidden as a committed form (2026-09-16 owner policy).

## SN-pipeline refactor (2026-09-16)

Named the gate global `pauseActionListMode` (config/symbols.txt) and replaced
the cast. Probed variants (tools/decomp_probe.py, `-G0` = pause_post.o's
PRIVATE_COMPILE_FLAGS) against the 44-byte original:

| form | result |
| --- | --- |
| `extern int` plain, default split | 2-word diff: `lui $v1; lw $v0,off($v1)` (base reg) |
| `extern int` + `section(".data")` | same 2-word diff |
| `extern volatile int` | same 2-word diff |
| `-G0 -mno-split-addresses` (TU flag) | load fixed (self-based pseudo), but 10-word diff: the flag also makes the list `la` pseudo unsplittable, so EGC drops the delay-slot `lui listB` hoist, emits `beq` (opcode 4) instead of `beql` (opcode 20), nops the delay slot, and moves the stores apart — the original schedule needs SPLIT list lui/addiu, conflicting with the unsplittable load |
| `asm volatile("lw %0, pauseActionListMode" : "=r"(v))` | **exact 44-byte match** |

Mechanics: EGC emits the bare `lw $v0, pauseActionListMode` pseudo for the asm
output operand (register chosen by the same allocation as the plain load, v0
here). ps2eeas is single-pass and, with no preceding `.extern` (EGC emits
`.extern` only for small-data symbols, and `-G0` has none) and the reference
outside a noreorder block, expands it in place to the self-based `lui $v0,
%hi; lw $v0, %lo($v0)` pair — the original's words. The rest of the function
keeps the default-split RTL, so the `beql` + delay-slot `lui listB` +
`b`/`addiu listA` schedule is unchanged. Precedent for load/store-bearing asm
with a symbol: transition.cpp (HelpMsgCount store). Final probe:
`decomp_state/probes/pause_setPauseActionList_v1_final.cpp`.

## Verification

- Standalone probe of the final form matched all 44 bytes
  (`decomp_state/probes/pause_func_0021A1B0_v4.cpp`,
  `tools/decomp_probe.py`, default flags).
- `decomp-verifier` independent check after implementation: full
  `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf` byte-identical; 11/11 words objdump-verified against
  the reference; no stale `func_0021A1B0` references anywhere.
- Count 750 → 749.
- 2026-09-16 refactor to the named symbol: probe matched all 44 bytes;
  incremental `make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf`
  byte-identical; `decomp_status` count unchanged (function was already
  matched, no INCLUDE_ASM removed).

Iteration probes: `pause_func_0021A1B0.cpp` (v1, literal if/else — movz diff),
`_v2.cpp` (symbol tables, 5-word diff: base reg + polarity), `_v3.cpp` (cast +
`!= 0` — first full match), `_v4.cpp` (final project-style form — match).
