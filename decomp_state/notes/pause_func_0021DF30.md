# func_0021DF30 (vram 0x21DF30, file 0x11EEB0, 36 bytes) — MATCHED 2026-09-11

Decompiled as `pause_resetMenuEntry(PauseMenuEntry*)` in
`code/game/pause_post.cpp` (was `INCLUDE_ASM` at line 154).

```
lui   at, 0x4049          ; hi(PI)
ori   at, at, 0x0FDB      ; PI = 0x40490FDB
mtc1  at, $f0             ; $f0 = PI
move  v0, zero            ; return 0
sw    zero, 0x48(a0)      ; field_48 = 0
swc1  $f0, 0x38(a0)       ; field_38 = PI (float)
sw    zero, 0x34(a0)      ; field_34 = 0
jr    ra
sw    zero, 0x44(a0)      ; field_44 = 0   (jr delay slot)
```

## Identity / context

A pause-menu callback. All three xrefs are DATA (function-pointer table
entries at 0x1CEF88, 0x1D0E30, 0x1D0EF0). In every table it is grouped with
`func_0021DF58` (always adjacent) and the ship-menu / moby callbacks
`func_0021DF98` (camera-angle/PI), `func_0021E110`, `func_0021E230`,
`func_0021E608`, followed by a `0x00000002` count word. It is the "reset"
callback of that family: it clears `field_34`/`field_44`/`field_48` and sets
the `field_38` angle to PI. Sibling `func_0021DF58` reads the `field_44`
pointer and calls `pause_refreshMobyTimestamp` (func_00225530), confirming the
moby-timestamp context of the shared struct.

Struct (only the touched fields are known):

```c
typedef struct {
    u8 pad[0x34];
    u32 field_34;   // = 0
    float field_38; // = PI
    u8 pad_3c[8];
    u32 field_44;   // = 0  (pointer per func_0021DF58/98)
    u32 field_48;   // = 0  (float per func_0021DF98 swc1)
} PauseMenuEntry;
```

## Codegen findings

### `-G0` is required for this TU (float-constant inlining)

The PI constant `0x40490FDB` (3.14159265f) needs a 3-instruction materialise
(`lui`+`ori`+`mtc1`). With the project default `-G8`, EGC **pools** any
float constant whose low-16 bits are non-zero via `lwc1 $fX,-16384(gp)` from
the `.lit4` pool, and inlines only lo16==0 constants (2-instr `lui`+`mtc1`).
`-G0` inlines **all** float constants. The original inlines PI, so the TU must
be `-G0`.

Evidence the original `pause_post.o` was built `-G0`:
- Matched sibling `draw/func_001F2070.s` (default `-G8`) only ever inlines
  lo16==0 constants (0x44800000, 0x3F800000, 0x45000000, 0x3A800000,
  0x41800000).
- Non-matching `pause_post` siblings (e.g. `func_0021DF98`) inline BOTH
  3-instr constants (PI 0x40490FDB, 0.3f 0x3E99999A) and 2-instr (3.0f
  0x40C00000) — i.e. no pooling.
- All 20 matched `pause_post` functions are float-free and use absolute
  (`lui`+`lw`) addressing, so `-G0` (vs `-G8`) changes nothing for them.

Added to Makefile: `$(OBJ_DIR)/game/pause_post.o: PRIVATE_COMPILE_FLAGS = -G0`
(appended after `COMMON_COMPILE_FLAGS`, so the later `-G0` wins). Verified safe
by a full `make` + `cmp` with this function still `INCLUDE_ASM` (parity held),
then with it decompiled.

### Int-store rotation reproduces the original tail

The three int stores (field_34/44/48) are emitted by EGC in the fixed
`[A,B,C] -> [C,A,B]` rotation (the float store stays mid-sequence; the last
int store lands in the `jr ra` delay slot). Source order
`[field_34, field_38, field_44, field_48]` therefore produces binary order
`[0x48, 0x38(float), 0x34, jr, 0x44]` — the original. (Same 3-constant-store
permutation family documented in `music_Unpause__Fv.md` / `space_func_0022E188.md`.)

### Symbol placement

The function-pointer tables hold the raw address 0x21DF30, and the linker
script (Splat) names the entry `func_0021DF30`. EGC 2.95.2's cfront rejects
`asm("...")` on a function *definition* (parse error before `{`), so the
project's declare-then-define idiom is used: a forward declaration carries
`asm("func_0021DF30")`, the definition is plain. This keeps the symbol
`func_0021DF30` (lands at 0x21DF30) while the source name is descriptive.

## Verification

- `tools/decomp_probe.py ... --flags=-G0` -> match:true, 0 differences, 36 bytes.
- Full `make -j4` + `cmp build/boot_elf.elf assets/boot_elf.elf` -> parity OK.
