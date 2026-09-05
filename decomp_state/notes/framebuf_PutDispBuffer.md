# PutDispBuffer__Fv (0x001FB2A8) — BLOCKED

## Body

```c
void PutDispBuffer() {
    func_00121E40(*(void**)0x15EEB8);
}
```

- `func_00121E40` is an `alabel` inside `func_00121E3C` in
  `code/_generated/sce/lib.s` — a real linkable symbol (lib.s is assembled
  directly by the Makefile, not via INCLUDE_ASM). Declare
  `extern "C" void func_00121E40(void* p);`.
- `D_0015EEB8` is a writable pointer in section `.lit4`
  (`code/_generated/build/data/core.lit.lit4.s`), `.float 0` at rest — it holds
  the display-buffer context set elsewhere.

## What matches

With the **constant-cast idiom** (`*(void**)0x15EEB8`, not the named symbol
`D_0015EEB8`), EGC emits the correct base-register form:

```
lui  a0, 0x16          ; 0x3C040016  %hi(0x15EEB8) rounded
lw   a0, 0xEEB8(a0)    ; 0x8C84EEB8  %lo
jal  func_00121E40
nop
```

(Using the named `extern s32 D_0015EEB8` instead gives `lui v0; lw a0,0(v0)` —
wrong base register, the documented menu-family load idiom.)

The C++ function must be named `PutDispBuffer` (unsuffixed) so cfront mangles
it to the binary symbol `PutDispBuffer__Fv` (naming it `PutDispBuffer__Fv`
double-mangles to `PutDispBuffer__Fv__Fv`).

## Why it doesn't match

Order. Original:

```
addiu sp,sp,-0x10
lui   a0, %hi(D_0015EEB8)
lw    a0, %lo(D_0015EEB8)(a0)
sq    ra, 0(sp)
jal   func_00121E40
nop
lq    ra, 0(sp)
jr    ra
addiu sp,sp,0x10
```

EGC 2.95.2 (project flags):

```
addiu sp,sp,-0x10
sq    ra, 0(sp)        <- prologue fixed unit
lui   a0, %hi
lw    a0, %lo(a0)
jal   func_00121E40
nop
lq    ra, 0(sp)
jr    ra
addiu sp,sp,0x10
```

The original delays the `sq ra` until AFTER the argument `lui/lw`; EGC always
emits `addiu sp; sq ra` as a fixed prologue pair at the top. No C form tried
(direct cast, named symbol, `void* p = ...; f(p);`) moves the `sq ra` after
the body loads. This is an EGC prologue-scheduling difference from the
original Insomniac compiler, not a source-structure issue.

## Revisit when

If compiler flags are revisited (AGENTS.md: flags may not match Insomniac's
yet), re-test whether a different -O / scheduling flag makes EGC interleave
the argument materialization before the ra save.
