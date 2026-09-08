# func_00222F58 (vram 0x222F58, 0x30 bytes) — MATCHED 2026-09-08

Single-call wrapper: `self->f48 = func_00225CD8(self->f48); return 0;`.
Reachable only through a function-pointer table (DATA xrefs at
0x001D17EC, 0x001D183C, 0x001D4D74, 0x001D4F54), so its return type is not
pinned by any direct caller — the `move v0,zero` in the epilogue shows it
returns 0 as an `int`.

```cpp
extern "C" int func_00225CD8(int param_1);

typedef struct {
    int pad[18];   // f48 at offset 0x48
    int f48;
} PauseCallback;

extern "C" int func_00222F58(PauseCallback *self) {
    self->f48 = func_00225CD8(self->f48);
    return 0;
}
```

## Key facts (EGC 2.95.2, default project flags)

- 0x20 frame, saves ra at 0x10(sp) and s0 at 0x0(sp); s0 holds `self`
  (`move s0,a0`).
- The argument load `lw a0,0x48(s0)` is hoisted into the `jal` delay slot;
  the callee return is written straight back to the same field
  `sw v0,0x48(s0)` (read-modify-write of `self->f48`).
- Epilogue is the 3-independent-op tail `lq ra; move v0,zero; lq s0; jr ra;
  <addiu sp,sp,0x20>` — matched exactly.
- The callee `func_00225CD8` is still a nonmatching INCLUDE_ASM later in the
  same TU; an `extern "C" int func_00225CD8(int);` prototype before the
  wrapper is what makes the call site generate correctly (empty/stub body of
  the callee is unaffected by the prototype).

All 12 words match the original; the `jal` is an object-relative reloc in the
`.o` that resolves at link to 0x0C089736 (target 0x225CD8). Full boot parity
passes (`cmp build/boot_elf.elf assets/boot_elf.elf`).
