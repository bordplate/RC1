# Remaining Probe Results

The remaining attempts retained their assembly fallbacks. No compiler patch
was justified by these results. Reproducible sources are in ../probes/ and
commands are described in ../compiler_workflow.md.

## Final Session Verification

Four successful replacements: menu_pointIsClockwise (formerly func_00208818),
menu_restoreSelection (formerly func_002088A8), scTag2, and func_00207690.
Their standalone relocated probes match and their fallbacks are removed.
The menu segment is split at exact function boundaries to isolate conflicting
pre-RA/post-RA scheduler flags without changing prior matches.

`make clean && make split && make -j2` followed by
`cmp build/boot_elf.elf assets/boot_elf.elf` passes. Both images have SHA-256
`e050581032e4bb3f20341307da5b69b76f1574910519155380ea771e55c3c0c9`.
The edge and callback positive controls pass; the callback pre-RA-disabled
negative control differs at the expected seven positions. `--complete`
correctly fails because other nonmatching functions remain.

## func_00207690

Matched 2026-09-06. Corrected semantics: `if (x >= 190) return D_0013D3D8 != 0;`
then `return 58.5f <= y;`, with arguments `(int, float, float, float)` so the
compared argument arrives in f14. The original's explicit NOP between `mtc1`
and `c.le.s` is an EE COP1 hazard. A plain C candidate remains one word short,
but this compiler accepts the following scheduling constraint and matches all
64 bytes:

```cpp
float threshold = 58.5f;
asm volatile("nop" : : "f"(threshold));
return threshold <= y;
```

The FPU input operand makes the compiler materialize `threshold` before the
inline NOP; the asm emits the hazard NOP itself. This is not a post-build word
patch. The candidate matches under the existing menu-only
`-fno-schedule-insns` flag, and full boot parity passes. Keep the FPU operand:
an unbound `asm("nop")` is scheduled before `lui`/`mtc1` and does not match.

## menu_restoreSelection (func_002088A8)

Retested the correct struct-field form in probes/menu_callback.cpp.
`-fno-schedule-insns2` plus an explicit fixed `$a0` selected temporary gets all
instruction order and register choices exactly right. The safe isolation is now
implemented: Splat boundaries at file offsets `0x109798` and `0x109850` create
`menu_callbacks.cpp` around the conflicting slice, while the prefix and suffix
retain `-fno-schedule-insns`. The generated linker script places all three
objects consecutively. Standalone comparison and clean full boot parity pass.

## PutDispBuffer__Fv

The natural free C++ function `void PutDispBuffer(void)` DOES mangle to
`PutDispBuffer__Fv`, verified by the probe resolving that symbol. No invented
class, unused `this`, or manual mangled name is needed.
Both -fno-schedule-insns2 and -fargument-noalias-global still place sq ra
before the address materialization: three differing instruction positions,
same 36-byte function length. The historical note's size 40 includes padding.
Fallback retained. The actual original sequence has lui/lw before sq ra.
