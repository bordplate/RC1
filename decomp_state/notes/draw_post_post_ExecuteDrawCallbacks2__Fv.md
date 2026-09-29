# ExecuteDrawCallbacks2__Fv (MATCHED 2026-09-29)

Target: `code/game/draw_post_post.cpp` (was INCLUDE_ASM at line 605),
`code/_generated/nonmatchings/game/draw_post_post/ExecuteDrawCallbacks2__Fv.s`,
0x78 bytes, vram 0x1F4808. Matched and now lives in C at line 606.

## Semantics
Runs the SECOND per-draw-phase callback list: for `i` in `0..drawCallback2Count`,
calls `drawCallback2Funcs[i](drawCallback2Args[i])`, re-reading the count after
every call. Byte-identical 0x78-byte clone of `ExecuteDrawCallbacks3__Fv`
(0x1F46C8); see `notes/draw_post_post_ExecuteDrawCallbacks3__Fv.md` for the
shared family analysis (all four clones now matched, family complete).

## The matching form
```cpp
void ExecuteDrawCallbacks2(void) {
    int i;
    for (i = 0; i < drawCallback2Count; i++) {
        ((DrawCallbackProc)drawCallback2Funcs[i])(drawCallback2Args[i]);
    }
}
```
Plain indexed for-loop on the already-declared `extern u32 drawCallback2Funcs[]`,
`extern u32 drawCallback2Args[]`, `extern int drawCallback2Count;` — the
u32[] + `(DrawCallbackProc)` cast form used by the matched list-1 sibling
(`ExecuteDrawCallbacks__Fv`), NOT the typed-array form of clones 3/4: the
already-matched `AddDrawCallback2` (0x1F47B8) stores its `u32 func` parameter
into `drawCallback2Funcs[]`, so the array declaration stays `u32[]` and the
executor carries the cast instead. Both forms were probed 120/120 before the
edit (probes in the working dir, since cleared); the cast form avoids touching
any matched code. NO pins, NO barriers, NO volatile, NO private flags.

Also removed the redundant `extern int drawCallback2Count;` in the
AddDrawCallback2 block (already declared with the other three counts at the
top of the callback section) — declarations emit no code, no layout effect.

## Verification
- `decomp_probe.py`: first probe 120/120 words, 0 differences (cast form).
- `make split` reclassifies the symbol to
  `code/_generated/matchings/game/draw_post_post/ExecuteDrawCallbacks2__Fv.s`.
- `tools/tu_assembler_diff.py`: draw_post_post.o 71/71 functions match.
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf`: pass.
- `decomp_status.py --count`: 616 -> 615.
