# func_00201200 (0x201200, 0xA8 = 168 bytes) — BLOCKED 2026-10-12

Source: `code/game/hud_post_post2_post3_tail.cpp:69` (still `INCLUDE_ASM`).
Splat symbol `func_00201200` (stripped ELF: no original name recoverable).
The HUD message text printer — sibling of the matched `Hud_DrawMsgBar`
(func_00201128) at 0x201128, its sole caller being `Hud_DrawChannels`
(func_001FF780) in `code/game/hud_post_post2_drawchannels.cpp`.

## Semantics (byte-verified)

`void Hud_DrawMsgText(int x, int y, int color, u8* text)` — 4 params
(a0,a1,a2,a3). Draws, in order:

```c
int alpha = color >> 24;                 // sra into s0, saved across call 1
alpha = min(alpha, 0x50);                // slti v1,s0,81 + movz s0,v0(0x50),v1
int cx = FontPrintCenterLarge(x+1, y+1, color & 0xFF000000, text, -1) - 0x20;
Hud_DrawMsgBar(cx, y - 8, (x - cx) * 2, 0x20, alpha);   // func_00201128
FontPrintCenterLarge(x, y, color, text, -1);
```

- Drop shadow: `FontPrintCenterLarge(x+1, y+1, color & 0xFF000000, text, -1)`
  (offset +1,+1; color = alpha byte only, RGB zeroed → black shadow).
- Bar: `Hud_DrawMsgBar(cx, y-8, (x-cx)*2, 0x20, alpha)` centered on the text
  anchor x, extending 0x20 beyond each edge, at the clamped alpha.
- Main text: `FontPrintCenterLarge(x, y, color, text, -1)`.

Register map (verified): s0=alpha, s1=x(a0), s2=y(a1), s3=color(a2),
s4=text(a3); frame 0x60. The mask `color & 0xFF000000` is folded by EGC
directly into a2 (the 3rd arg of call 1): `lui a2,0xff00; and a2,s3,a2`.
The clamp constants: `li v0,80` (0x50), `slti v1,s0,81`, `movz s0,v0,v1`.

## Best candidate: 6 word-diffs

`working/hud_func_00201200/v13.cpp` (production default flags, GNU
assembler) = 0xA8 bytes / 6 diff words, with s1=x/s2=y reproduced via
register pins:

```c
register int rx asm("$17") = x;   // s1
register int ry asm("$18") = y;   // s2
```

The natural no-pin form ALWAYS allocates y→s1, x→s2 (swapped vs the
original's x→s1, y→s2); the pins are required to fix the allocation.
(The matched sibling Hud_DrawMsgBar needs no pin because its first call
passes x as a1 with a0 a derived value; here x is the a0 of the first
call, and EGC assigns it the higher s-register.)

## Residual: EGC 2.95.2 default-scheduler prologue tie-break

The 6 diffs are entirely the relative order of the six free prologue
instructions (addresses 0x201224–0x201244):

| instr            | original | candidate |
|------------------|----------|-----------|
| A lui a2 (mask)  | 0x201224 | 0x201238  |
| B slti (clamp)   | 0x201234 | 0x201224  |
| C move s4 (text) | 0x201238 | 0x201234  |
| D and a2 (mask)  | 0x20123c | 0x201244  |
| E movz (clamp)   | 0x201240 | 0x20123c  |
| F sq ra          | 0x201244 | 0x201240  |

Original orders **A B C D E F** (mask `lui` hoisted to the earliest free
slot; `and a2` before `movz`). Every candidate orders **B C A E F D**
(`slti` hoisted to the earliest slot; mask tight `lui`@0x201238 /
`and`@0x201244; `movz` before `and a2`).

Root cause: the mask is the 3rd arg of call 1, so its `lui` lives in the
call's arg-setup RTL, which is AFTER the clamp statement. EGC therefore
starts the clamp pair (B,E) before the mask pair (A,D). The original
behaves as if the mask were earlier than the clamp — an ordering this
EGC build does not reproduce for this shape.

## Exhausted attempts (none change the 6 diffs / make it worse)

- Every source statement reordering (alpha/mask/clamp permutations):
  identical 6 diffs.
- Mask as a local before/after the clamp (`int shadow = color &
  0xFF000000;`): EGC folds into a2, still starts the clamp pair first.
- Precomputing x+1/y+1 and the bar-call args into locals, both orders.
- Ternary clamp forms (statement / local / self-assign / reversed): no
  change or worse (16 diffs).
- `-fno-schedule-insns` (12 diffs), `-fno-schedule-insns2` (21 diffs).
- Input-only zero-byte asm barrier after a mask local (42 diffs; forces
  the mask into a live register, breaks the 0x60 frame).
- expert (GPT-6 Astra): pin the mask to `$6` with a tied barrier before
  the AND (`register unsigned int maskedColor asm("$6")=0xFF000000u;
  asm volatile("" : "+r"(maskedColor)); maskedColor &= color;`) → 41
  diffs / 0xAC (hard-register pin perturbs the allocation).
- last-resort (GPT-5.6 Sol): direct nonvolatile extended-asm `lui` into
  the `$6`-pinned shadow placed before the clamp
  (`asm("lui %0, 0xff00" : "=r"(shadow)); ... shadow = (unsigned)color &
  shadow;`) → identical 6 diffs / 0xA8.

Both escalations applied and mechanically diffed; neither changes the
scheduler's choice to hoist `slti` over the mask `lui`. Confirmed
unreproducible EGC 2.95.2 default-scheduler tie-break.

## Verification

- `tools/decomp_probe.py` (v13): 0xA8 / 6 diffs (best form).
- Source unchanged (still `INCLUDE_ASM`); `make` + `cmp
  build/boot_elf.elf assets/boot_elf.elf` passes (parity preserved).
