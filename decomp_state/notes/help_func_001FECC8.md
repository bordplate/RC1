# func_001FECC8 → messageIdCodeTableFind (code/game/help.cpp) — MATCHED

Date: 2026-10-08. VRAM 0x1FECC8, size 0x64 (100 bytes, 25 instrs), no stack frame.

## What it does
`int messageIdCodeTableFind(int value, int column, u16* idOut)`:
search the 150-entry `messageIdCodeTable` for the record whose `id` (column 0) or
`code` (column 1) equals `(s16)value`, returning the record index, or -1 if none match.
When column is 1 (a code search) and `idOut` is non-null, the matched record's `id` is
also stored into `*idOut`, mapping a text-code back to its message id.

- `value = (s16)value` (SEH). `base = messageIdCodeTable`, `p = base + column`
  (start at record 0's `column`-member). `i = 0`, `q = 2`.
- `do { if (*p == value) { q = column ? (i<<2) : q; if (idOut) *idOut = *(u16*)(q+base);
  return i; } i++; q += 4; p += 2; } while (i < 150); return -1;`
- On a match with column 1, q becomes `i<<2 = 4i` (record i's byte offset), so the
  store reads record i's FIRST member (the id). With column 0 the callers pass
  idOut = 0 so the store is skipped.

## Data layout
- messageIdCodeTable @ 0x199710 (.data): 150 × { s16 id; u16 code; } = 600 bytes,
  immediately followed by s_Paradox_this_message_does_not @ 0x199968. The `code`
  column is the contiguous text-handle range 0x526F..0x5302 (21103+k); the `id`
  column holds categorized message ids (0-5, 1000-1003, 2000-2014, 20017-20019,
  ...); the final two records are zero.

## Callers
- func_001FED30 (help.cpp, INCLUDE_ASM; `extern "C" void func_001FED30(int msgId)` in
  freeze.cpp): `f(msgId, 0, 0)` — find the record by message id, push the returned
  index onto a shown-messages byte list (D_00141E08, count at D_0015EE30, initial 1)
  with de-dupe / move-to-end. Called by matched mode_freezeInit (freeze.cpp) as
  `func_001FED30(MSG_AUTOSAVE_WARNING)`.
- func_0021A328 (pause screen, INCLUDE_ASM): `f(code, 1, &id)` — map a level
  text-code back to its message id for msg_string.

## Match (byte-for-byte; full boot ELF cmp passes)
The winning source (see help.cpp) and the tie-breaks it had to reproduce:
1. The match-path `movn a3,v1,a1` condition is **`a1` (column)**, NOT `a2` (idOut)
   — the raw word 0x0065380b decodes to `movn a3,v1,a1` (confirmed by objdump). So
   the ternary is `q = column ? (i<<2) : q`, and it only if-converts to a movn
   (never a branch) in this form.
2. A NAMED `base` local (= messageIdCodeTable) is required: computing the store base
   from the table directly (no local) makes EGC copy the base into a second register
   (+move +nop, 108 bytes).
3. Pre-loop barrier `asm volatile("" : : "r"(p), "r"(base) : "$3", "$7")` forces
   base→t2, p→t0, and leaves q in a3 (the original's home registers).
4. The i<<2 temp must land in v1: `register int r asm("$3") = i << 2;` before the
   ternary. Without it EGC puts the temp in v0 and q in v1 (5 residual words).
5. The store address is an INTEGER add with q on the left, `*(u16*)(q + (u32)base)`,
   so EGC emits `addu v0, a3, t2` (q left / base right) matching the original; the
   pointer form `(char*)base + q` normalizes to `addu v0, t2, a3` (base left).
6. `asm volatile("");` before the increments keeps `i++` in the back-edge block
   instead of EGC stealing it into the exit-branch delay slot (plain bne, not bnel).

Pinning q to $7 itself (with or without the barrier) kills the movn (EGC emits a
branch), so q's home register is obtained via the barrier, not a pin.
last-resort GPT-5.6 Sol used (2026-10-08): its key correction was the `movn ...a1`
(column, not idOut) decode, which dropped the candidate from 108B/26-diff to 100B with
a correct CFG; the residual 5 register-only words were then closed locally (items 4-5).
