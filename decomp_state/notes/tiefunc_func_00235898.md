# func_00235898 (0x235898, 44 bytes) — tiefunc

Matched 2026-09-07 with default flags, first probe attempt.

```cpp
extern "C" void LightTies(u16*);
extern "C" u16 D_001E3200[];
extern "C" u16 D_001E4400[];

extern "C" void func_00235898(void) {
    LightTies(D_001E3200);
    LightTies(D_001E4400);
}
```

Purpose: light both tie light banks. `LightTies` (0x237370, tieproc) walks a
0xFFFF-terminated array of u16 light indices and sets up VU1 lighting; the two
globals at 0x1E3200/0x1E4400 (in .data) are the two banks' index lists. The
sole caller (draw, 0x1F39D0) runs this plus func_00235840 when the "both ties"
flag is set, otherwise the single-bank pair `LightTies(0x1E3200);
PatchTieGifs();`.

Scheduling: EGC hoists the first argument's `lui a0, %hi(D_001E3200)` ABOVE
`sq ra,0(sp)` and parks the `addiu a0,a0,%lo(...)` in the first `jal` delay
slot; the second call is the plain `lui; jal; <addiu>` shape. This is the
constant-address call-arg case — unlike PutDispBuffer's blocked `lui/lw`
pointer-load body, the hi/lo pair schedules with the hi before `sq ra` under
default flags, no scheduler override needed.

Object relocations: R_MIPS_HI16/LO16 on D_001E3200/D_001E4400, R_MIPS_26 on
LightTies at both jals. Probe decomp_state/probes/tiefunc_00235898.cpp matches
all 44 bytes; clean build + cmp byte-for-byte; count 751->750.
