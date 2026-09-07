# func_00221908 (vram 0x221908, file 0x122888, 0x28 bytes) — MATCHED 2026-09-07

Pause-module update callback (vtable at vram 0x001D2260 =
{ 0x221908 (update), 0x221930 (draw) }).

```c
extern "C" int D_0013CB04 __attribute__((section(".data")));
extern "C" int* D_001D5BF8 __attribute__((section(".data")));
extern "C" int D_001D22F8 __attribute__((section(".data")));

extern "C" int func_00221908(void) {
    if (D_0013CB04 & 0x40) {
        D_001D5BF8 = &D_001D22F8;
    }
    return 0;
}
```

If pause-state word D_0013CB04 has bit 0x40 set, point D_001D5BF8 (a hot
pointer written by ~30 pause-module sites; field 8 of the struct based at
0x1D5BF0) at the data table D_001D22F8. Returns 0.

## Key scheduling facts (EGC 2.95.2, default flags)

- Return type: `int` + `return 0;`, NOT void. The original ends
  `jr ra; daddu v0,zero,zero` (delay-slot zeroing); a void body compiles to
  `jr ra; nop` (verified against the matched menu family func_00208E68 tail).
  The `daddu` is NOT hoisted here — with the conditional-store body shape it
  lands in the epilogue delay slot.
- Condition load: plain global symbol with `__attribute__((section(".data")))`
  (out of the gp window, 0x13CB04 < 0x156C00) reproduces the original
  `lui a0, %hi; lw v0, %lo(a0)`. The constant-cast form `*(int*)0x13CB04`
  instead allocates the base to v0 (`lui v0; lw v0, off(v0)`) — 2 word diffs.
  Extension of the menu-family load-side observation (0x15EEB4, in-window):
  OUT-of-window plain globals land the base in a0, in-window ones in v0/v1.
- Store: `D_001D5BF8 = &D_001D22F8;` with both as section-attributed symbols
  reproduces `lui a0, %hi(dst); addiu v0, v0, %lo(src); sw v0, %lo(dst)(a0)`
  with the lui-dst BEFORE addiu-src. Writing the value as the constant
  `0x1D22F8` flips the order to `addiu; lui at; sw` — 3 word diffs. So a
  lui/addiu materialized value stored to an out-of-window symbol wants the
  SOURCE written as a symbol (address of a defined global), not an immediate.
- All three globals need the section attribute: plain externs at out-of-window
  addresses fail the link with R_MIPS_GPREL16 truncation (reproduced).

Probe: decomp_state/probes/pause_00221908.cpp, all 40 bytes match
(tools/decomp_probe.py, --define for the three symbols). pause.o object words
+ relocations agree with the original; full `cmp build/boot_elf.elf
assets/boot_elf.elf` passes.
