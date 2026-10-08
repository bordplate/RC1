# mobyfunc DeleteMoby (0x20C828, 0x58)

Matched 2026-10-08. Source: `code/game/mobyfunc.cpp`. The boot-ELF symbol is
the UNMANGLED `DeleteMoby` (handwritten-era name); a C++ free function would
mangle, so the source uses `asm("DeleteMoby")` on the declaration (cfront
cannot parse `asm()` on a definition line — declaration-only override is the
project pattern, cf. MobyAssignRenderSlot).

## Semantics

Retires a moby instance:

- `state` (MobyInstance+0x20) = 0xFD if `moby < MobyInstanceEnd` (pool
  region), 0xFE if it is a permanent instance at or past the pool end.
  CreateMoby's spawn scan skips slots with `state < 0xFE`, so both stamps
  remove the slot from allocation; 0xFF (the other skip class) is the
  init/empty marker.
- `unk1` (MobyInstance+0x38, u64) = `worldUpdateTime + 2`. CreateMoby's
  scan also skips slots whose `unk1` is ahead of `worldUpdateTime`, so the
  +2 stamp keeps the slot skipped for a couple of world updates.
- Calls `UpdateMobyGrids(moby, 0x80807F7F)` — the handwritten grid walk in
  `game/mobyproc` (C linkage, unmangled, `extern "C"`). It stores the stamp
  at MobyInstance+0xA0 and stops matching grid cells whose +0xAC stamp
  equals it, retiring the moby's collision grid cells.

Callers (all discard the void return): 0x1EBEAC, 0x225544, 0x22EC04,
0x22FA7C, 0x22FA90.

## Codegen findings

1. **In-window .bss globals load self-based via plain externs (no section
   attribute).** `MobyInstanceEnd` (0x15ff1c) and `worldUpdateTime`
   (0x15f60c) sit in the GP window; the original loads each as a
   self-based `lui v0; lw v0, off(v0)` pair. The pre-existing fallback
   declarations carried `__attribute__((section(".bss")))`, which makes
   EGC emit a two-register split (`lui v1; lw v0, 0(v1)`) the scheduler
   then tears apart (lw after `addiu sp`). Plain externs (32-bit pointer /
   s32, both <= 8 bytes) are classified small-data under the project
   `-G8` and compile to the bare pseudo `lw r, sym`, which ps2eeas
   expands in place to the original self-based pair. This is the
   2026-09-16 `-G8` bare-pseudo behavior; see
   notes/pause_func_0021CB00.md.
2. **`state` is `u8`, not `s8`.** The original materializes the stamps as
   `addiu v0, 0, 0xFD` / `0xFE` (positive immediates), and every other
   reader (CreateMoby's `lbu` + `u8 nextState`, func_0020C880's `lbu`)
   treats the byte unsigned. Declaring `s8` makes EGC fold 0xFD to -3 and
   emit `addiu v0, 0, -3` (word 0x2402FFFD) — one word off on each branch.
   The struct field was corrected to `u8` in code/include/mobyfunc.h; no
   production code relied on the signed read.
3. **0x80807F7F splits unsigned.** The second arg materializes as
   `lui a1, 0x8080; ori a1, a1, 0x7F7F`. Passing the literal to a `u32`
   parameter reproduces the unsigned `ori` low half; a signed constant
   context would instead emit `addiu a1, a1, 0x7F7F` (0x24A57F7F), one
   opcode nibble off — keep the parameter unsigned.
4. **Register map falls out naturally** with the above: moby in v1 from
   entry (`daddu v1, a0, 0` after the prologue), both globals in v0, stamp
   in a1, the `sd v0, 0x38(v1)` timestamp store lands in the jal delay
   slot, `sq/lq ra` at 0(sp), 0x10 frame. No pins, no private flags;
   default `-G8 -O2 -ffast-math -fno-exceptions -snas`.

## Verification

- `tools/decomp_probe.py working/DeleteMoby/probe.cpp
  code/_generated/nonmatchings/game/mobyfunc/DeleteMoby.s DeleteMoby`
  → 88/88 bytes, `match: true`, no differences.
- Built object objdump: all 22 words identical to the original; relocations
  R_MIPS_HI16/LO16 → MobyInstanceEnd, worldUpdateTime and R_MIPS_26 →
  UpdateMobyGrids.
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf` passes;
  `tools/decomp_status.py --count` 559 → 558.
