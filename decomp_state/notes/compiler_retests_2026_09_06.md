# Remaining Probe Results

The following attempts retained their assembly fallbacks. No compiler patch
was justified by these results. Reproducible sources are in ../probes/ and
commands are described in ../compiler_workflow.md.

## Final Session Verification

Two successful replacements: menu_pointIsClockwise (formerly func_00208818)
and scTag2, 36 bytes each. Their standalone relocated probes match; their
fallbacks are removed from production source. The menu-only flag also required
the documented equivalent func_002089A8 store reorder.

`make clean && make split && make -j2` followed by
`cmp build/boot_elf.elf assets/boot_elf.elf` passes. Both images have SHA-256
`e050581032e4bb3f20341307da5b69b76f1574910519155380ea771e55c3c0c9`.
Six tooling tests pass. The edge positive control passes and default-flag
negative control fails at the expected four positions. `make -n -W` on an
included menu assembly file schedules menu.o recompilation. `git diff --check`
passes. `--complete` correctly fails: 769 nonmatching functions remain,
including 16 associated with blocker records by stable source/name identity.
All five finished attempts sent their single mobile notification successfully.

## func_00207690

Corrected semantics: `if (x >= 190) return D_0013D3D8 != 0; return 58.5f <= y;`
with arguments `(int, float, float, float)`. The older blocked.json note
misreads the 16-bit immediate 0x00BE as -66. It is +190. The constant 58.5f
and f14 placement are correct. Ghidra currently has no function at this address.

Default flags produce 60 bytes versus 64: one missing NOP between mtc1 and
c.le.s. Compiler -S output contains `li.s` and a COMMENT `#nop`, not an
explicit mtc1 or nop. Macro expansion/hazard handling is therefore an
assembler-level investigation too, not just a scheduler issue.
The following isolated tests still fail: default, -mcpu=r5900, -Wa,-m5900,
-Wa,-g, -Wa,-O0, -Wa,-mips1. The latter also changes `daddu` to `addu`.
`-fno-schedule-insns -mcpu=r4000` additionally changes the FP comparison
opcode. Do not enable these flags globally or insert an unexplained inline nop.

## func_002088A8

Retested the correct struct-field form in probes/menu_callback.cpp.
`-fno-schedule-insns` alone still differs in scheduling. Explicitly creating
a local struct pointer fixes the register choices but not the order.
`-fno-schedule-insns2` alone gets the instruction order exactly right, but
uses v1 instead of a0 for the 0xBC load and 0x1C store (two word differences).
Disabling both passes changes base/constant order and the trailing stores.
Useful next experiment: source lifetime/type changes under nopost, then
evaluate the impact on the rest of menu.cpp; do not assume menu's currently
verified pre-RA-off configuration also solves this callback.

## PutDispBuffer__Fv

The natural free C++ function `void PutDispBuffer(void)` DOES mangle to
`PutDispBuffer__Fv`, verified by the probe resolving that symbol. No invented
class, unused `this`, or manual mangled name is needed.
Both -fno-schedule-insns2 and -fargument-noalias-global still place sq ra
before the address materialization: three differing instruction positions,
same 36-byte function length. The historical note's size 40 includes padding.
Fallback retained. The actual original sequence has lui/lw before sq ra.
