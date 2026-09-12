# Camera_Exit__FP9UpdateCam (0x001EC3D8, 0x44 bytes / 17 words) — MATCHED 2026-09-12

C++ free function in code/game/camera.cpp, `void Camera_Exit(UpdateCam*)`.

## Semantics (confirmed)

Per-level camera-type vtable dispatch: look up the `exit` callback for the
camera's type and call it.

```
if (lvlCamVtbl[cam->camType].exit)
    lvlCamVtbl[cam->camType].exit(cam);
```

- `lvlCamVtbl` = 0x001E8C00, the `lvl.camvtbl` section (rodata). Entry size is
  0x14 (5 words); the boot level's single entry is `{0xFFFFFFFF, 0, 0, 0, 0}`,
  i.e. all callbacks null (the main-menu camera has no per-type behavior).
  Each level overlay supplies its own table at the same address, so the symbol
  is level-owned (hence the `lvl.` section prefix).
- `cam->camType` is the signed `short` at UpdateCam offset 0x8C (the original
  loads it with `lh`, so the field must be signed).
- `UpdateCam` is 0xA0 bytes (UpdateAllCameras__Fi iterates 0x2F entries of
  0xA0 at 0x187410 with active flags at 0x189B50; BackupCurrentCam copies
  0xA0 bytes). The global current camera is `curCam` (0x1870C0).
- Sole caller: `UpdateAllCameras__Fi` (FUN_001ec420 at 0x1EC420, `jal
  Camera_Exit__FP9UpdateCam; ...` with a0 = curCam) — the very start of the
  update, before the camera-switch loop.

## Vtable layout (established from the four dispatcher functions)

All four index the table as `&lvlCamVtbl[id]` (id = `lh 0x8C(cam)`, 3-operand
`mult` by 0x14, `addu`, field load, `beqz`, `jalr`):

| offset | type                          | used by                            |
|--------|-------------------------------|------------------------------------|
| 0x00   | int (boot level: -1)          | not referenced by any dispatcher   |
| 0x04   | int (*)(UpdateCam*, UpdateCam*) | Camera_ActivationCheckPriority (result compared to -1 and 1) |
| 0x08   | void (*)(UpdateCam*)          | Camera_runSetupToNewCam            |
| 0x0C   | void (*)(UpdateCam*, float)   | UpdateAllCameras__Fi (float = cam->0x30 in f1; cam->0x64 = cam->0x30 is stored after the call) |
| 0x10   | void (*)(UpdateCam*)          | Camera_Exit (this function)        |

Declared as `struct UpdateCamVtbl` in camera.cpp; the other three slots stay
INCLUDE_ASM so the slot signatures are documentation for their future
decompilation.

## Exact original (17 words)

```
0:  27BDFFF0  addiu  sp, sp, -0x10
4:  0080282D  move   a1, a0          # cam kept in t0 across the address setup
8:  7FBF0000  sq     ra, 0(sp)
C:  24040014  li     a0, 0x14        # entry-size mult constant hoisted into a0
10: 3C02001F  lui    v0, 0x1F        # %hi(lvlCamVtbl) signed split
14: 84A3008C  lh     v1, 0x8C(a1)    # camType
18: 24428C00  addiu  v0, v0, -0x7400 # %lo(lvlCamVtbl)
1C: 00641818  mult   v1, v1, a0      # R5900 3-operand mult (result in v1, not LO)
20: 00431021  addu   v0, v0, v1
24: 8C420010  lw     v0, 0x10(v0)    # .exit
28: 10400004  beqz   v0, +16
2C: 7BBF0000  lq     ra, 0(sp)       # (taken-path delay slot)
30: 0040F809  jalr   v0
34: 00A0202D  move   a0, a1          # (jalr delay slot: re-materialize cam)
38: 7BBF0000  lq     ra, 0(sp)       # (fall-through path)
3C: 03E00008  jr     ra
40: 27BD0010  addiu  sp, sp, 0x10
```

Splat prints the mult word's comment hex reversed (0x18186400); the real word
is 0x00641818 (`op=0, rs=v1, rt=a0, rd=v1, sa=32, funct=0x18`) — the R5900
fixed-point 3-operand mult storing the 32-bit product in `rd`. Verify with the
PS2 objdump, not Splat's hex field.

## The C shape and why it matched on the second try

```cpp
void Camera_Exit(UpdateCam* cam) {
    void (*fn)(UpdateCam*) = lvlCamVtbl[cam->camType].exit;
    if (fn)
        fn(cam);
}
```

with `extern UpdateCamVtbl lvlCamVtbl[];` and `lvlCamVtbl = 0x001E8C00;` in
config/symbols.txt. A plain SYMBOL reference (not a constant cast) gives the
signed %hi/%lo split the original uses (0x1F / -0x7400); the R_MIPS_HI16/LO16
relocs in camera.o confirm it.

First attempt failed parity with ~1539 differing bytes starting at 0x1EBCF0:
the C function had been placed at the TOP of camera.cpp (after
BackupCurrentCam), so EGC emitted it FIRST in camera.o's .text and every
subsequent function (ExecuteCamPostUpdFuncs onward) shifted by 0x44. The
original object's .text is in address order, so the C definition must sit in
the source at its address position — between the
Camera_ActivationCheckPriority and UpdateAllCameras__Fi INCLUDE_ASMs — exactly
where the placeholder was. (Structs/declarations are code-free and can stay
anywhere before use.)

## Verification

- Function bytes at 0x1EC3D8 compared equal (68 B) between build and original.
- `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf` pass (symbols.txt changes touch generated files, so a
  clean run was used).
- The rename D_001E8C00 → lvlCamVtbl propagated through the generated asm of
  the other three dispatcher functions (still INCLUDE_ASM) and the
  lvl.camvtbl data label; full parity confirms no side effects.
