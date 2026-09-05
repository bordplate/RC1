# snd_UnkFunction_0012eb00 (0x0012EB00)

## What it does

```c
extern void snd_FlushSoundCommands(void);
extern int D_0015ECC4;

void snd_UnkFunction_0012eb00(void) {
    D_0015ECC4 = 0;
    snd_FlushSoundCommands();
}
```

Clears the GP-relative global `D_0015ECC4` (0x15ECC4, a pointer in `.lit4`,
`.float 0` at rest — the 989snd stream/VAG callback slot) and then flushes the
pending IOP sound command queue via `snd_FlushSoundCommands` (0x12DC80). Called
from many sites (movie setup 0x1EB95C, game sound/update 0x216868/0x2168E8/
0x21691C/0x22D6A4/0x22DCD8) — a "clear slot + flush" helper. The pinned binary
symbol name is `snd_UnkFunction_0012eb00` (config/symbols.txt:32); kept as-is.

## Codegen notes

- `D_0015ECC4` is declared **plain** `extern int` (no section attribute, no
  cast) so EGC emits a GP-relative store: `sw zero, -0x7F3C(gp)`
  (R_MIPS_GPREL16; 0x15ECC4 − gp 0x166C00 = −0x7F3C). Same idiom as the
  matched `D_0015EC84`/`D_0015EC80` in this file.
- **Scheduling surprise (useful):** EGC emits an *independent* constant store
  (not feeding a call argument) **before** the `sq ra` prologue store:
  `addiu sp; sw zero,off(gp); sq ra; jal; nop; lq; jr; addiu sp`. This matches
  the original. Contrast with PutDispBuffer (blocked), where the body was a
  `lui/lw` that feeds the call argument and EGC instead emitted `sq ra` first.
  So EGC's prologue/body interleaving depends on whether the body instruction
  is argument materialization (kept after sq ra) or an independent store
  (hoisted before sq ra).

## Verification

- Object 8/8 words identical; R_MIPS_GPREL16 → D_0015ECC4 (−0x7F3C from gp),
  R_MIPS_26 on .text section symbol → snd_FlushSoundCommands (0x12DC80).
- Full `make` + `cmp`: identical. Count 797 -> 796. decomp-verifier MATCH.
