# func_001F46C8 — ExecuteDrawCallbacks3__Fv (MATCHED 2026-09-29)

Target: `code/game/draw_post_post.cpp` (was INCLUDE_ASM at line 554),
`code/_generated/nonmatchings/game/draw_post_post/func_001F46C8.s`, 0x78 bytes,
vram 0x1F46C8. Matched and renamed; the function now lives in C at line 562.

## Semantics
Runs the "pre effects" per-draw-phase callback list: for `i` in
`0..drawCallback3Count`, calls `drawCallback3Funcs[i](drawCallbackArgs[i])`,
re-reading the count after every call. Registered by overlay/level code via the
per-phase AddDrawCallback family; executed by the draw pipeline
(`DrawDebugProfiler`, 0x1F39D0) before the VU-effect list.

## Family (all renamed 2026-09-29)
Four byte-identical 0x78-byte executor clones, one per callback list:

| list | count | funcs | args | executor |
|------|-------|-------|------|----------|
| 1    | drawCallbackCount 0x15F464 | drawCallbackFuncs 0x18DB40 | drawCallbackArgs 0x18DC40 | ExecuteDrawCallbacks__Fv 0x1F4650 (matched 2026-09-29, u32[] + cast form) |
| 2    | drawCallback2Count 0x15F468 | drawCallback2Funcs 0x18DD40 | drawCallback2Args 0x18DE40 | ExecuteDrawCallbacks2__Fv 0x1F4808 (matched 2026-09-29, u32[] + cast form) |
| 3    | drawCallback3Count 0x15F46C | drawCallback3Funcs 0x18DF40 | drawCallback3Args 0x18E040 | **ExecuteDrawCallbacks3__Fv 0x1F46C8 (matched 2026-09-29, typed-array form)** |
| 4    | drawCallback4Count 0x15F470 | drawCallback4Funcs 0x18E140 | drawCallback4Args 0x18E240 | ExecuteDrawCallbacks4__Fv 0x1F4740 (matched 2026-09-29, typed-array form) |

FAMILY COMPLETE: all four clones matched 2026-09-29 with the plain indexed
for-loop (no pins/barriers/volatile/flags). Lists 1 and 2 use the `extern
u32[]` + `(DrawCallbackProc)` cast form because their already-matched
AddDrawCallback* registrator stores a u32 into the funcs array; lists 3 and
4 use the typed `extern DrawCallbackProc[]` form. Both forms probe 120/120.

All names added to `config/symbols.txt` (function mangled names + the
previously-unnamed funcs/args arrays); the three remaining INCLUDE_ASM
references in the source were renamed to match the regenerated .s files.

## The matching form
```cpp
typedef void (*DrawCallbackProc)(u32);
extern DrawCallbackProc drawCallback3Funcs[];
extern u32 drawCallback3Args[];

void ExecuteDrawCallbacks3(void) {
    int i;
    for (i = 0; i < drawCallback3Count; i++) {
        drawCallback3Funcs[i](drawCallback3Args[i]);
    }
}
```
Plain indexed for-loop. NO pins, NO barriers, NO volatile, NO private flags
(production defaults: `-G8 -O2 -ffast-math -fno-exceptions -snas`).
First probe on this form: 120/120 bytes.

EGC lowers `funcs[i](args[i])` to exactly the original layout:
- the for-loop's FIRST check (`0 < count`) is the single `blez` guard; `i=0`
  (`move s0,zero`) sits in the prologue before it, and the `sq` order is
  s0@0, ra@0x30, [i=0], s2@0x20, blez / delay-slot s1@0x10;
- both subscripts strength-reduce to pointer locals: s1=funcs, s2=args
  (the first-subscripted array gets s1), materialized as `lui v1,%hi(funcs);
  lui v0,%hi(args); addiu s2,v0,%lo(args); addiu s1,v1,%lo(funcs);` — the two
  independent high-half producers emit in the original's order (funcs first),
  sharing page 0x19 with negative lo immediates;
- the first fn load hoists to `lw v1,0(s1)` before the loop (no padding nop);
- loop body: `i++`, `lw a0,0(s2)`, `s1+=4`, `jalr v1`, delay-slot `s2+=4`,
  count reload, `slt v0,s0,v0`, `bnel`, delay-slot fn reload `lw v1,0(s1)`;
- epilogue: lq ra/s2/s1/s0, `jr ra`, delay-slot `sp+=0x40`.

## Why the earlier pointer-based wall was form-specific
`notes/draw_post_post_func_001F4650.md` (this session's earlier attempt on
clone 1) ruled out ~40 pointer-based variants (explicit p/a locals with
pins/barriers/flags, for(;;)+break) and left a 2-byte wall: the two setup
`lui` high-halves emit in consumer order, the original emits funcs-first.
The indexed form was never among the ruled-out variants: with
`funcs[i](args[i])` EGC creates the pointer locals itself during
subscript-strength-reduction, and the high-half order comes out funcs-first
with correct temps (funcs→v1, args→v0) and the original s1/s2 allocation —
all for free. Verified on clone 3 (this match) and probed on clone 1
(120/120, `probe_sib` in the working dir, since cleared): the same plain
indexed form is expected to match all remaining clones; the list-1 blocker's
"unrepresentable in pure C" conclusion (last-resort GPT-5.6 Sol,
2026-09-29) is superseded for the indexed form.

## Verification
- `decomp_probe.py` indexed form: 0 differences (120/120 words).
- `make split` reclassifies the symbol to
  `code/_generated/matchings/game/draw_post_post/ExecuteDrawCallbacks3__Fv.s`.
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf`: pass.
- `decomp_status.py --count`: 619 -> 618.
