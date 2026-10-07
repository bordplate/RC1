# Help_AdvanceState (0x1FDC08) — MATCHED (2026-10-07)

Function: `code/game/help.cpp` : `Help_AdvanceState` (VRAM 0x1FDC08, size 0x88, 34 words).
Splat placeholder was `func_001FDC08`; the level-overlay callers reference that unmangled
label, so the C++ definition keeps it via `asm("func_001FDC08")`.

## Semantics
The Help message state machine transition. Reads the current `state` (unsigned) from
`g_helpState` (0x1996D0, renamed from `D_001996D0`; `HelpMsgCount`@0x1996FC is its +0x2C
member) and rewrites it based on the current value:

```
void Help_AdvanceState(void):
    state = g_helpState.state           // unsigned
    if (state < 8) switch (state):
        case 0:   g_helpState.field_0x24 = -1; return;     // clear active-message field
        case 1,2,3: g_helpState.state = 7; counter = 0; return;
        case 4:   g_helpState.state = 6; counter = 4 - counter; return;
        case 5:   g_helpState.state = 6; counter = 0; return;
        case 6,7: break;                                 // fall out, state unchanged
```

`state >= 8` also leaves the state unchanged. The function is **void**: the values that end in
`$v0` (7, 6, -1) are store temporaries, and states 6/7 return whatever the jtbl base left in
`$v0`, so no defined integer return is implementable — an `int` return is a misread.

Struct (fields beyond +0x28 and the exact meaning of field_0x24 not yet established):
```
struct HelpState {
    unsigned int state;      // +0x00 (0x1996D0)
    int counter;             // +0x04
    int field_0x08;          // +0x08 ... +0x1C
    int field_0x20;          // +0x20
    int field_0x24;          // +0x24 (set to -1 in case 0)
    // +0x28 .. +0x3C  (HelpMsgCount is +0x2C)
};
```

## KEY CODEGEN LEARNINGS
1. **The COMMON-block merge requires separate case 3.** The original merges the four similar
   cases into one block `COMMON: state = v0; counter = 0; fall to TAIL`, with cases 1/2/3 each
   emitting `b COMMON` + `li v0,7` in the delay slot and case 5 `li v0,6` then falling through.
   Writing `case 1: case 2: case 3:` as ONE combined case (or a shared value variable +
   common-after-switch) does NOT reproduce this; EGC keeps the bodies separate. Writing
   `case 3:` as its OWN case with a duplicated `state=7; counter=0; return;` body makes EGC
   emit the two distinct `b COMMON; li v0,7` blocks that match. (Contrast the freeze
   mode_freezeInit, whose cases are all distinct and match with source-order bodies.)
2. **`void` return + `unsigned` state.** A signed `int state` emits `slt` + a redundant `sltu`
   and a separate `return 0` path; the original uses a single `sltiu` guard. `unsigned int`
   fixes the comparison. The `if (state < 8) { switch }` form (not `if (state >= 8) return 0;`)
   matches the prologue/epilogue merge.
3. **Dense-switch jtbl in `.rodata` (the tooling).** EGC emits the 8-entry jtbl into
   `help.o(.rodata)`, but the original keeps it INSIDE the data segment at 0x1E7A20
   (`jtbl_001E7A20`, 0x20 bytes = the 8 case-target words 0x1FDC38/48/48/50/58/78/88/88).
   Splat models that region as a raw blob, so it was carved out as a `data_help` segment
   (ROM 0xE89A0 / VRAM 0x1E7A20) with a `data_suffix2` segment (0xE89C0 / 0x1E7A40) after it,
   splitting the old `data_suffix` (0xE8870/0x1E78F0 .. 0xE89A0). `tools/patch_rodata_ld.py`
   (generalized from `patch_freeze_rodata_ld.py` to handle both holes) places `help.o(.rodata)`
   in the `.data_help` output section, guarded by a top-level
   `ASSERT(ADDR(.data_help)+SIZEOF(.data_help) == 0x1E7A40, ...)`, and drops it from the `.text`
   rodata region. Same mechanism as the freeze jtbl (see notes/freeze_mode_freezeInit.md).
4. **Probe caveat for switch functions.** `decomp_probe.py` links only `.text`, so the two
   `lui %hi(jtbl)` / `addiu %lo(jtbl)` setup words (0x1FDC1C, 0x1FDC24) always differ in the
   probe (the probe's jtbl lands in `.rdata` at a different address). They match in the real
   build once `.rodata` is placed at 0x1E7A20. Judge switch functions by the full build, not
   the probe's jtbl words.

## Verification
`decomp_probe.py` (C form, `--define g_helpState=0x1996d0`): candidate 0x88, only the two jtbl
address words differ. `make clean && make split && make -j2` then
`cmp build/boot_elf.elf assets/boot_elf.elf` => byte identical (patch_rodata_ld places
`help.o(.rodata)` at 0x1E7A20, 32 bytes). Count 570 -> 569.
