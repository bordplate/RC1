# func_0021EAF0 (0x0021EAF0, 44 bytes), pause.cpp

Matched 2026-09-07 with default flags (pause.cpp TU).

## Semantics

Pause action-table callback (referenced as a function pointer from the two
data tables at 0x1D1DD4 and 0x1D24C4). It reloads the object's 0x44 field
through `func_00225530` and stores the result back:

```c
p[0x11] = func_00225530(p[0x11]);   // offset 0x44 == 0x11 * 4
return 0;
```

`func_00225530` (0x225530, still INCLUDE_ASM) treats its argument as a
sub-object pointer: `if (arg) { func_0020c828(); *(u32*)(arg+0x38) =
D_0015F60C; }`. So the 0x44 field holds a sub-object pointer and this
callback re-derives it.

## Replacement (code/game/pause.cpp)

```cpp
extern "C" int func_00225530(int);

extern "C" int func_0021EAF0(int* p) {
    p[0x11] = func_00225530(p[0x11]);
    return 0;
}
```

`int* p` (not a named struct) matches the existing `func_0021A318` style and
avoids committing to the full pause-object layout, which is not yet
reverse-engineered. The callee is forward-declared `extern "C"` because its
INCLUDE_ASM definition sits later in the same TU (line ~312).

## Codegen

EGC reproduces the 12 words exactly with default flags: 0x20 frame,
`sq ra,16(sp); sq s0,0(sp); move s0,a0`; the `lw a0,0x44(s0)` argument load
lands in the `jal` delay slot; `sw v0,0x44(s0)` after the call; `move
v0,zero` (return 0) between `lq ra` and `lq s0`; `addiu sp,sp,0x20` in the
`jr` delay slot. The `int` callee prototype is required (its result is
consumed by the store). The `jal` is a normal symbol relocation to
`func_00225530` (defined later in the TU); the linked word resolves
correctly.

## Verification

`tools/decomp_probe.py` (decomp_state/probes/ equivalent, candidate in
/tmp) reports all 44 bytes matching with `--define func_00225530=0x225530`.
Clean `make -j2` then `cmp build/boot_elf.elf assets/boot_elf.elf` passes
byte-for-byte. decomp_status count 749 -> 748.
