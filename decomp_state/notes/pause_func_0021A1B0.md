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

extern "C" int D_001D4810[];
extern "C" int D_001D4840[];

extern "C" int SetPauseActionList(PauseActionMode* mode) {
    if (*(int *)0x15EE90 != 0)
        mode->actionList = (u32)D_001D4810;
    else
        mode->actionList = (u32)D_001D4840;
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
   `extern int` symbol load gave `lui v1; lw v0,off(v1)` (base in v1), while
   the original reuses v0 for base and value (`lui v0; lw v0,off(v0)`).
   The constant-address cast `*(int *)0x15EE90` reproduces the original
   (same fix as the menu 0x208E68 family).

## Verification

- Standalone probe of the final form matched all 44 bytes
  (`decomp_state/probes/pause_func_0021A1B0_v4.cpp`,
  `tools/decomp_probe.py`, default flags).
- `decomp-verifier` independent check after implementation: full
  `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf` byte-identical; 11/11 words objdump-verified against
  the reference; no stale `func_0021A1B0` references anywhere.
- Count 750 → 749.

Iteration probes: `pause_func_0021A1B0.cpp` (v1, literal if/else — movz diff),
`_v2.cpp` (symbol tables, 5-word diff: base reg + polarity), `_v3.cpp` (cast +
`!= 0` — first full match), `_v4.cpp` (final project-style form — match).
