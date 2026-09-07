# func_00239750 (vram 0x239750, file 0x13A6D0, 44 bytes) — MATCHED 2026-09-07 as setupViewContext

## Identity

`func_00239750` is a byte-identical clone of the already-matched
`func_001F7978` (code/game/draw.cpp, vram 0x1F7978). All 12 instruction
words are equal (verified by diffing the Splat hex rows of
`code/_generated/matchings/game/draw/func_001F7978.s` against
`code/_generated/nonmatchings/game/vendor/func_00239750.s`; the jal
targets are absolute so the words agree across locations):

```
addiu      $29, $29, -0x10
sq         $31, 0x0($29)
jal        PutDrawBufferLarge__Fv
  nop
jal        InitViewContext__Fv
  nop
jal        UpdateViewContext__Fv
  nop
lq         $31, 0x0($29)
jr         $31
  addiu     $29, $29, 0x10
```

The C replacement is therefore the same proven body from draw.cpp:

```c
void PutDrawBufferLarge();
void InitViewContext();
void UpdateViewContext();

extern "C" void setupViewContext(void) {
    PutDrawBufferLarge();
    InitViewContext();
    UpdateViewContext();
}
```

## What it does

Re-initializes the 3D rendering context: PutDrawBufferLarge (0x1FB2D0)
points the GTE draw-context chain at DAT_00160F00+0x30 (registers
0x30000009/0x50000009), InitViewContext (0x1F2C60) computes the
projection/view scratch from the camera globals (DAT_001518D0/D2), and
UpdateViewContext (0x1F2D98) computes the full view matrix (position,
FOV, aspect).

## Caller context

Single xref: `jal` at 0x239BD0 inside func_00239780 (vendor.cpp, a large
GTE/VU scene renderer with a 6-pass loop). The call sits between two VU
micro-program loads via 0x1FB740 (a0=a1=0x200): phase 1 draws with one
micro-program, `setupViewContext` resets the GTE draw buffer + view,
phase 2 loads a second micro-program and draws again.

## Naming

Renamed `func_00239750` -> `setupViewContext` (extern "C"). Added
`setupViewContext = 0x00239750;` to config/symbols.txt (new `// vendor`
section) and re-ran `make split`, which regenerated func_00239780's
generated asm with `jal setupViewContext` — without the split rerun the
link failed with undefined reference to `func_00239750`. The draw.cpp
clone was left as `func_001F7978` (already committed under that name;
renaming it would be a separate, out-of-scope change).

## Verification

- Standalone probe (run before the rename, with symbol `func_00239750`):
  `python tools/decomp_probe.py
  decomp_state/probes/vendor_00239750.cpp
  code/_generated/nonmatchings/game/vendor/func_00239750.s
  func_00239750 --out /tmp/opencode/probe-00239750` — result:
  `"match": true, "differences": []` (44/44 bytes, default flags).
- Clean build: `make clean && make split && make -j2` succeeded.
- `cmp build/boot_elf.elf assets/boot_elf.elf` passes; bytes at file
  offset 0x13A6D0..+0x2C verified identical between build and original.
- Nonmatching count 753 -> 752.
