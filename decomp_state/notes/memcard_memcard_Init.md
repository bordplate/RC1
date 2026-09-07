# memcard_Init (0x20AC58, 48 bytes)

Matched 2026-09-07 with default flags, first probe attempt.

```c
extern "C" int func_001233F0(void);
extern char D_001E8360[];
extern "C" void STUB_printf(const char* fmt, ...);

extern "C" void memcard_Init(void) {
    if (func_001233F0() != 0)
        STUB_printf(D_001E8360);
}
```

Notes:
- Original symbol is unmangled (`memcard_Init` in config/symbols.txt), mixed with
  C++-mangled siblings in the same file (memcard_Save__Fii); `extern "C"` on the
  definition is the correct spelling.
- func_001233F0 is an SCE library function (code/_generated/sce/lib.s): IOP
  RPC that inits the memcard lib, returns 0 on success / negative error code
  on failure. Declared int; only its zero/nonzero result is used (beqz $v0).
- D_001E8360 is the rodata string "ERROR: could not init memcard lib"; out of
  the gp window so EGC emits absolute lui/addiu into $a0, matching the original.
- Shape: 0x10 frame with sq/lq ra; `jal` first (nop delay), `beqz` with `lq ra`
  in its delay slot, redundant second `lq ra` on the fall-through before the
  shared `jr ra` epilogue — same shape as matched func_002169C0.
- Single caller FUN_00201650 (boot init) ignores the return value, so void is
  correct; an int-returning form would need no extra v0 setup here anyway, but
  void matches the observed use.
- Probe: decomp_state/probes/memcard_init.cpp, all 48 bytes match.
  Clean `make clean && make split && make -j2` + full `cmp` passes.
