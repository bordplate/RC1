# func_001EBE60 (0x1EBE60) — RESOLVED 2026-09-22: dead-tail byte preservation

NOT a function. A 4-byte (0x4) unreachable fragment immediately after the
MATCHED parent `Cam_InterpValues__FffPffff` (0x1EBD78, 0xE4 bytes, frame 0x30,
C definition at code/game/camera.cpp:71), plus one 4-byte alignment nop before
the next real function `Camera_handleCollWithHero__FiP9UpdateCam` (0x1EBE68).

```
0x1EBE60: 0x27BD0050  addiu sp, sp, 0x50   (the 0x4 "function")
0x1EBE64: 0x00000000  nop                  (alignment padding before 0x1EBE68)
```

The parent epilogue completes at 0x1EBE54/0x1EBE58 (`jr ra` + `addiu sp,sp,0x30`
delay slot); 0x1EBE5C is the alignment gap (emitted by the `.align 3` ahead of
this fragment). The fragment has no prologue, no branch, no return, and cannot
be a function. Its single unit is `addiu sp,sp,0x50` — a stack deallocation of
0x50 that does NOT match the parent's 0x30 frame, consistent with the dead-tail
family where tail units differ from the parent frame (see 989snd_func_0012E078.md).

## Identity

Member of the ~46 EGC dead-tail "phantom function" artifact family (see
help_msg_string__Fi.md "Systemic" section, 989snd_func_0012E078.md, and the
sibling camera_func_001EBD60.md). This is the simplest family shape: a lone
`addiu sp,sp,N` + nop, with no data-store or zero-move residue.

Unlike the func_001EBD60 sibling (whose parent ExecuteCamPostUpdFuncs is
BLOCKED), this fragment's parent Cam_InterpValues is already matched in C. When
that parent was decompiled (commit 769b653), the post-epilogue dead tail was
left behind as an orphan INCLUDE_ASM; this resolution clears it.

## Deadness (verified 2026-09-22)

- tools/deadness_scan.py 0x1EBE60: 0 reference(s) (scans core.text and .text for
  jal/j/relative-branch targets, plus all non-text sections for 32/64-bit words
  equal to the address).
- Ghidra headless: no function at or containing 0x1EBE60; zero xrefs.
- The next symbol 0x1EBE68 (Camera_handleCollWithHero) is independently entered
  with its own prologue (`addiu sp,sp,-32; lui v0,0x18; ...`).

## Why no C form matches

- Unreachable: no C function can occupy 0x1EBE60 without a terminator that would
  shift 0x1EBE68.
- A standalone `addiu sp,sp,0x50` cannot be compiled from C (a function needs a
  prologue/return); local EGC 2.95.2 never emits a bare post-`jr` stack
  adjustment as its own unit.
- The parent (matched) cannot supply the tail: its matched C ends at its own
  epilogue, and EGC will not append the 0x50 unit after it.

## Resolution (per the 2026-09-18 AGENTS.md dead-tail policy and the
camera.cpp func_001EBD60 / actuator.cpp / stash.cpp file-scope asm precedent)

The orphan `INCLUDE_ASM("code/_generated/nonmatchings/game/camera",
func_001EBE60)` in camera.cpp was replaced by a file-scope `asm(...)` block that
emits the two words verbatim (`.word` form, no mnemonics) between
Cam_InterpValues' definition and Camera_handleCollWithHero's INCLUDE_ASM, so
camera.o's .text keeps the exact 8 bytes (gap nop + addiu + trailing nop). The
block carries `nonmatching func_001EBE60, 0x4` + `glabel func_001EBE60` so the
symbols remain defined in camera.o exactly as the generated assembly defined
them, and `.align 3` + the trailing nop preserve the 8-byte alignment into
Camera_handleCollWithHero.

Per the policy this is NOT a decompilation target and gets NO blocked.json
entry; the target drops out of the status tool's count (676 -> 675).

## Verification

- `nm -n build/code/game/camera.o`: func_001EBE60 / func_001EBE60.NON_MATCHING
  at object 0x1D0, Camera_handleCollWithHero at 0x1D8 (unchanged layout).
- `objdump -d` of build/boot_elf.elf 0x1EBE5C-0x1EBE68: gap nop, `addiu
  sp,sp,0x50`, nop, then the next prologue — byte-identical to the original.
- `make` + `cmp build/boot_elf.elf assets/boot_elf.elf`: byte-identical.
- The change lives only in camera.cpp source, so it survives `make split`
  regeneration by construction.
