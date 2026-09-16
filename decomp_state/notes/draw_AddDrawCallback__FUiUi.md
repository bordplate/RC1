# AddDrawCallback__FUiUi (0x1F4600) — required constant-address cast

Refactor entry `draw_AddDrawCallback__Fuu` (hardcoded-data-address) resolved
2026-09-16 as a documented, required GNU codegen artifact. The function stores
and reloads the callback-list counter through `*(int*)0x15F464` although
`drawCallbackCount = 0x0015F464` exists in config/symbols.txt. The cast is
retained; the reason it cannot be replaced by the named symbol is recorded
here.

## Original accesses

The original (80 bytes) uses self-based absolute accesses to the counter:

```
lui  $6, %hi(drawCallbackCount)
lw   $6, %lo(drawCallbackCount)($6)      # load idx
...
lui  $1, %hi(drawCallbackCount)
sw   $5, %lo(drawCallbackCount)($1)      # store idx+1
```

## Probes (decomp_probe.py, `-G8 -O2 -ffast-math -fno-exceptions`)

Reference: `code/_generated/matchings/game/draw/AddDrawCallback__FUiUi.s`.

| Probe | Declaration | Assembler | Result |
| --- | --- | --- | --- |
| `draw_addcb_v1_plain_gnu.cpp` | `extern int drawCallbackCount;` | GNU | MISMATCH. EGC emits a single GPREL16 macro (`lw $6,drawCallbackCount` / `sw $5,drawCallbackCount`); GNU as expands it to one GP-relative instruction each → 72 B vs 80 B. |
| `draw_addcb_v2_data_gnu.cpp` | `extern int drawCallbackCount __attribute__((section(".data")));` | GNU | MISMATCH. Two-register absolute load (`lui $9; lw $6,%lo($9)`) plus a reallocated body (`move $8,$4` where the original has `daddu $7,$4,$0`; the Funcs/Args stores swap registers). |
| `draw_addcb_v3_data_nosplit_gnu.cpp` | same + `-mno-split-addresses` | GNU | MISMATCH. The count access becomes correct (GNU as expands the `lw $6,drawCallbackCount` pseudo to a self-based pair) but the body reschedules: the `daddu` moves drop, the Args store precedes the Funcs store, the count store goes through `$at` → 64 B vs 80 B. |
| `draw_addcb_v4_plain_snas.cpp` | `extern int drawCallbackCount;` | SN (ps2eeas) | **MATCH 80/80.** ps2eeas expands the GPREL macro in place to a self-based absolute `lui/lw` (and `lui/sw`) pair because EGC emits the `.extern` declaration at the end of the file, after the reference. |

Control `draw_occl_snas.cpp` (enableOcclusion/disableOcclusion, 0x1F61E8/0x1F61F8):
their `jr $ra` delay-slot GPREL stores to `drawOcclusionEnabled` sit inside
EGC's `.set noreorder/.set nomacro` block and stay GPREL16 under ps2eeas; both
match under SN. Confirms the delay-slot mechanism is not the source of the
SN-migration failure below.

## Why draw.o cannot migrate to the SN assembler

A full build with draw.o assembled by ps2eeas (source unchanged, constant cast
still in place) fails `cmp` at byte 0xA1265. `tools/tu_assembler_diff.py
build/code/game/draw.o build/boot_elf.elf` reports every function different,
but the root cause is a single size delta:

- `func_001F0B88` (draw_resetTextureDmaState): 60 B under GNU as vs 48 B under
  ps2eeas. The original's do-while loop contains three NOPs
  (0x1F0BA8/0x1F0BAC/0x1F0BB0) between `addiu v1,v1,-1` and the `bgez` that
  GNU as emits for the `.set noreorder/.set nomacro` + `bgez` pattern; ps2eeas
  omits them.
- That 12-byte delta shifts the whole TU 0x10 early from 0x1F0BC8 on, so every
  later function compares against the wrong VMA (the 84/84 "differ" cascade).

Conclusion: the original draw.cpp was assembled with the GNU assembler, so
draw.o must remain one of the five GNU-compatibility TUs. The named-symbol form
that matches (plain extern under ps2eeas) is therefore unavailable for this TU,
and the constant-address cast is the only C form that matches under GNU.

## Family note

The counter is one of five per-draw-phase callback-list counters zeroed by
ResetDrawGlobals (0x1F37E8-0x1F380C): 0x15F464 (drawCallbackCount, named),
0x15F468, 0x15F46C, 0x15F470, 0x15F474 (all still D_ placeholders). The
add/call-all functions are 0x1F4600 (AddDrawCallback), 0x1F4650, 0x1F46C8,
0x1F4740, 0x1F47B8, 0x1F4808; the call-all functions are invoked from the
draw pipeline dispatcher at 0x1F39D0 under various DAT_0018a2xx / DAT_0015F434
gates. Their individual phase roles are not yet established, so the sibling
counters are left unnamed pending that investigation (a separate, nonmatching
scope).
