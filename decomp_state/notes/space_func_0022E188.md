# func_0022E188 (vram 0x22E188, file 0x12F108, 28 bytes) — matched 2026-09-05

## Semantics

Space-transition setup. Stores the requested space id and raises two
pending flags consumed by the transition code:

- `D_0015F600` = param (space id; `DoSpaceTransition` tests `< 8` right
  next to `GameMode`@0x15F604 / `worldUpdateTime`@0x15F60C)
- `D_0015F5B0` = 1 (flag read by `Transition_DoTransition__Fv`)
- `D_0015F618` = 1 (pending flag; read first by func_00230EE8, cleared by
  func_00230F60 at level init)

`li v0,1` is just the constant store value; the function is VOID and all
six callers ignore v0 (verified: 0x2222E8, 0x2233C8, 0x223790, 0x223998,
0x1FD53C, 0x1FD5CC pass 0 or a loaded global, never use the return).

## Original body (file offset 0x12F108, vram 0x22E188, 7 words)

```
24020001  li    v0, 1
3c010016  lui   at, 0x16
ac24f600  sw    a0, -2560(at)      # 0x15F600 = param
3c010016  lui   at, 0x16
ac22f618  sw    v0, -2536(at)      # 0x15F618 = 1
03e00008  jr    ra
af8289b0  sw    v0, -30288(gp)     # 0x15F5B0 = 1 (delay slot)
```

## Replacement (code/game/space.cpp)

```cpp
extern "C" int D_0015F5B0;

extern "C" void func_0022E188(int param_1) {
    *(int*)0x15F600 = param_1;
    D_0015F5B0 = 1;
    *(int*)0x15F618 = 1;
}
```

## Refactor 2026-09-16 (renamed + globals named)

The function is now `space_beginLoad(int loadId)` (void) and all three
stores use named globals:

```cpp
extern int spaceLoadPending;      // 0x0015F5B0 (previously D_0015F5B0)
extern int spaceLoadId;           // 0x0015F600
extern int spaceLoadInProgress;   // 0x0015F618

void space_beginLoad(int loadId) {
    spaceLoadId = loadId;
    spaceLoadPending = 1;
    spaceLoadInProgress = 1;
}
```

Meaning (from xref sweep + Ghidra):

- `spaceLoadId` (0x15F600): the space id to load. Written here and by the
  two other transition entry points (0x1E9A84, 0x21E870); read by
  DoSpaceTransition (0x231FF0), which tests `< 8`, uses it to select the
  per-space scene setup, and copies it to 0x15EDC4 (current space) before
  calling the level init.
- `spaceLoadInProgress` (0x15F618): set together with the id; the 0x230EE8
  dispatcher routes on it (nonzero -> the case-4 per-frame update path),
  the per-frame space update (func_0022F778) re-asserts it, and the level
  init func_00230F60 clears it. Every other writer stores 0 or 1.

Codegen: `spaceLoadId` / `spaceLoadInProgress` must stay PLAIN externs.
The `.data` section attribute makes EGC emit each address as two
schedulable `lui` instructions (bases $3/$5, hoisted ahead of both
stores — a 6-word body with the wrong base registers). A plain extern
makes EGC emit one `sw r, sym` pseudo per store, which ps2eeas expands
IN PLACE (the reference precedes the end-of-file `.extern`) to the
original's `lui at / sw` pair. The gp store stays GPREL16 because EGC
emits it inside its `.set noreorder`/`.set nomacro` region. Statement
order `spaceLoadId; spaceLoadPending; spaceLoadInProgress` is the
verified scheduling order (finding 3 above).

Verification: object diff shows only the pending HI16/LO16/GPREL16
relocation fields differing from the original; the 28-byte slice at file
offset 0x12F108 is byte-identical after linking; full build +
`cmp build/boot_elf.elf assets/boot_elf.elf` pass.

## Codegen findings (IMPORTANT)

1. **Return type drives the constant register.** For an `int`-returning
   function EGC reserves v0 for the return value and materializes body
   constant stores into v1 — two separate `li` instructions (one per use
   site), no hoisting. The original has ONE `li v0,1` at the top used by
   both constant stores. That only happens for a VOID function, where v0
   is free for the body constant. All 8 `int`-returning source forms
   (inline literals, `int x = 1` at top/middle, chained assignment
   `a = b = 1`, `return (b = (a = 1))`) compile to the same 8-instruction
   2-`li` shape; only the void form collapses to the original 7 words.
   (Standalone EGC -G8 -O2 matrix, see attempts below.)
2. `$at` base on the absolute stores confirms constant-cast accesses
   (`*(int*)0x15F600`); the gp-relative third store confirms a plain
   `extern "C" int` (no section attribute, in gp window).
3. **Store scheduling with two cast stores + one gp store:** source order
   `cast600; gp5B0; cast618` reproduces the original machine order
   `cast600; cast618; jr; <gp5B0 in delay slot>` with the `li v0,1`
   hoisted to the top. The naive order `cast600; cast618; gp5B0` instead
   puts the gp store second (right after the first cast store) — wrong.

## Verification (mechanical)

- objdump of build/code/game/space.o `func_0022E188`: 8 instructions
  (7 + trailing nop), identical encodings to the original; GPREL16 reloc
  on D_0015F5B0 fills -0x7650.
- Raw 28-byte slice at file offset 0x12F108 built == original
  (`01000224 1600013c 00f624ac 1600013c 18f622ac 0800e003 b08982af`).
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- decomp_status --count: 813 -> 812.
