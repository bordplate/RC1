# Camera_HandleScreenFade (func_001EDA60) — matched 2026-09-25

Address 0x1EDA60, 0x48 bytes (18 words). Per-frame screen-fade update.
Matched on the first candidate with default camera-TU flags
(`-G8 -O2 -ffast-math -fno-exceptions -snas`); full boot ELF parity passes.

## What it does

Called from the level-cam per-frame update (0x1EDAA8) immediately after the
camera timer bump and immediately before Camera_updateCollMode
(func_001ED940) — the same slot Deadlocked uses for
`Camera_HandleScreenFade` (reference/dl/game_dl/camera.cpp), which is its
direct descendant (DL evolved it into a per-camera fadeIdeal/fadeSpeed/Delt
machine; RC1 keeps the single-global ancestor form).

```
cur = currentCamera.screenFade;          // Camera+0x258 (0x187198)
if (cur != 0.0f) {
    next = screenFade - cur;             // 0x15F43C
    screenFade = next;
    if (next <= 0.0f) {
        currentCamera.screenFade = 0.0f;
        screenFade = 0.0f;
    }
}
```

The camera's own fade value is consumed from the global screen fade; when the
global can no longer cover it, both are cleared. `screenFade` (0x15F43C) is
the black full-screen overlay value: draw dispatch (0x1F39D0, flag 0x40)
renders it as alpha = lrintf(clamp(v,0,1) * 128) via the overlay-rect helper;
the white overlay uses the sibling 0x15F440. Level load
(Transition_LoadWad, 0x1E9B10) sets it to 1.0f (fade in from black) and the
per-frame transition update (0x1EB0A8) decrements it by 0.0625 with a clamp
at 0.

## Naming

- `screenFade` = 0x15F43C (config/symbols.txt); `screenFadeGp` alias in
  config/linker_aliases.ld. Name follows Deadlocked's ScreenFade family.
- `Camera_HandleScreenFade` with `asm("func_001EDA60")` override: the boot
  ELF has no symbol at this address (Splat placeholder), same situation as
  the sibling Camera_updateCollMode.
- `Camera::screenFade` field at 0x258 in code/include/camera.h (was pad).
  Only this function touches the word in the boot ELF (xref-verified; no
  absolute 0x187198 data words in the image); level overlays supply the
  writes.

## The mixed-address-mode word (0x15F43C)

| site | original form | mode |
|---|---|---|
| `next = screenFade - cur` | `lwc1 $f0, -0x77C4(gp)` (bc1t delay slot) | GPREL |
| `screenFade = next` | `swc1 $f0, -0x77C4(gp)` (bc1f delay slot) | GPREL |
| `screenFade = 0.0f` | `lui at, 0x16; swc1 $f2, %lo(at)` | absolute |

Same mechanism as snd_BankLoadByLoc (decomp_state/notes/
989snd_snd_BankLoadByLoc.md): in this SN-assembler TU the plain `screenFade`
declaration is unseeded, so ps2eeas expands its reference self-based absolute;
the `screenFadeGp` alias is seeded with `asm(".extern screenFadeGp, 4")` at
the top of camera.cpp so its two references expand GPREL16. One alias is
enough here (no CSE between the two GPREL sites: the load feeds `next`, the
store stores `next`, and the absolute tail store uses the other symbol).

## Codegen notes

- The original interleaves `mtc1 $0, $f2` (0.0f constant) BETWEEN the
  `lui/addiu` pair that materializes `currentCamera`; local EGC did the same
  in the plain C form — no barriers needed.
- The 0x258 field is declared at its exact offset in struct Camera so the
  load/store stay single `lwc1/swc1 base,0x258(base)` instructions.
- The EGC split of currentCamera came out as hi=0x18 / lo=0x6F40 (not
  0x186F/0x40) — identical to the original's split.
- Relocations of the matched object: HI16/LO16 currentCamera, GPREL16
  screenFadeGp (x2, -0x77C4), HI16/LO16 screenFade (0x16/0xF43C).
