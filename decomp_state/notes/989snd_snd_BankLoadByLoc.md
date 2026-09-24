# snd_BankLoadByLoc (0x12DF20, 0x154) — matched 2026-09-24

Supersedes the 2026-09-17 BLOCKED state (constant-cast candidate v12, 20 diffs,
last-resort GPT-5.6 Sol exhausted): the forbidden static-address casts are no
longer needed — the absolute sites come from the plain unseeded declaration in
an SN-assembler TU, and the two GPREL sites from two seeded aliases. The old
5-access-form table and the do-while / fresh-return-load control-flow facts
still hold and are absorbed below.

CD bank-load-by-location: asks the CD RPC server (opcode `SND_IOP_CMD_CD_BANK_LOAD_BY_LOC`
= 0x03, 8-byte `{loc, offset}` params, 4-byte result) to load a sound bank at a CD
location, then waits for the IOP to write the result into `snd_cdCallbackArg`
(0x15ED00) and returns it. Reports an error string and returns 0 when a load is
already pending, the CD is still streaming, or the RPC call fails
(`snd_cdLoadError = 0x106`).

## Why a new TU (Splat boundary split)

The function sat in `989snd_mid.c`, a GNU-assembler TU. The original accesses the
4-byte result word `snd_cdCallbackArg` in two different address modes inside the
same function:

| site | original form | mode |
|---|---|---|
| sentinel store `= 0xFFFFFFFF` | `lui at, %hi; sw v1, %lo(at)` | absolute, at-based |
| RPC arg `&snd_cdCallbackArg` | `lui a5, %hi; addiu a5, %lo` | absolute, 2-instr |
| polling-loop load | `lui v0, %hi; lw v0, %lo(v0)` | absolute, self-based |
| post-RPC test load | `lw v1, -0x7F00(gp)` | GPREL |
| post-RPC return load (bne delay slot) | `lw v0, -0x7F00(gp)` | GPREL |

Under the GNU assembler no named declaration produces the absolute sites (bare
pseudo → GPREL; `.data` → split/cached-hi form). The self-based absolutes are the
signature of **ps2eeas single-pass expansion of unseeded bare pseudos** (see the
`ps2eeas is single-pass` note in AGENTS.md and `989snd_bankload.c`). The matched
sibling `snd_BankLoadFromEE_CB` already uses that mechanism in an SN TU, so the
function was moved to a new SN-assembler TU `989snd_mid_byloc.c` via a Splat
boundary at file offset 0x2EEA0 (VRAM 0x12DF20), preserving all addresses
(`989snd_mid.o(.text)` shrank 0x288→0x120; `989snd_mid_byloc.o(.text)` = 0x164
content + 4 zero-pad bytes). The `func_0012E078` dead tail (0x60/0x50
`addiu $sp` fragment after the function) moved with it as raw asm.

## EEGCC's small-data emission (the mechanism)

EEGCC 2.95.2 emits **bare pseudos** for small-data references
(`lw $2,snd_cdCallbackPending`, `sw $0,snd_cdLoadError`, `la $9,snd_cdCallbackArg`
for address-of) plus a trailing `.extern sym, 4` block at end of file. The
assembler decides the mode:

- **GNU gas** (multi-pass): the end-of-file `.extern` covers the whole file →
  every bare pseudo expands GPREL16. `la` on a small-data sym → `addiu r, gp, off`.
- **ps2eeas** (single-pass): a bare pseudo before any `.extern` declaration
  expands **self-based absolute** (`lui r, %hi; lw r, %lo(r)`; stores via `at`);
  after a seeded `asm(".extern sym, 4")` it expands GPREL16. Seeding is monotonic
  per symbol for the rest of the file.

So one plain declaration + seeded aliases reproduces the original's mixed modes:

```c
extern unsigned int snd_cdCallbackArg;          // unseeded → absolute (3 sites)
extern unsigned int snd_cdCallbackArgGp;        // seeded    → GPREL (test)
extern unsigned int snd_cdCallbackArgGpRet;     // seeded    → GPREL (return)
asm(".extern snd_cdLoadError, 4\n"
    ".extern snd_cdCallbackPending, 4\n"
    ".extern snd_cdCallbackArgGp, 4\n"
    ".extern snd_cdCallbackArgGpRet, 4");
```

`snd_cdLoadError` / `snd_cdCallbackPending` also need seeds (their original
accesses are GPREL; unseeded they would expand absolute under ps2eeas).
`snd_cdBankLoadRequest` (0x137B40) and `snd_cdRpcServer` (0x15EBE8) stay
`.data`-attributed (out of the GP window; explicit `%hi/%lo` RTL,
assembler-independent). The two aliases map to 0x15ED00 in
`config/linker_aliases.ld` (symbols.txt rejects duplicate addresses).

## The two-alias requirement (CSE defeat)

The post-RPC tail is `if (Gp != -1) return Gp;` followed by the polling
do-while. With ONE seeded name, EEGCC CSEs the return into `move v0, v1`
(v1 holds the test load); the original **reloads** the word in the bne delay
slot (`lw v0, -0x7F00(gp)`). Giving the return its own alias (`GpRet`) makes it
an independent load node → the reload, scheduled into the bne delay slot, and a
byte-for-byte match. `volatile` on the single name also forces the reload but
flips the inner branch direction (beq-to-loop + an extra materialized exit,
+8 bytes, outer test becomes BNEZ) — verified in probes p6–p9; the two-alias
form keeps the original's bne-to-epilogue layout.

## Other matched details

- Outer test is `if (call < 0)`; EEGCC emits the complement `bgez` to the
  success tail (original `0x04410008`).
- Polling is a **do-while** (`do FlushCache(0); while (arg == -1);`) — the
  original loop starts with the FlushCache call and loads/compares at the end;
  a `while (arg == -1) FlushCache(0);` pre-check made EEGCC emit an extra
  `bne` + dead `lq` (+0x10 bytes).
- `SND_CD_CALLBACK_ARG_NONE 0xFFFFFFFF` unsigned spelling (EGC lui/ori split).
- `0x106` → `SND_CD_LOAD_ERROR_RPC_FAILURE` (meaning unresolved; Deadlocked
  writes the same value into `gLocalLoadError` at the same failure point).
- Opcode 0x03 added to `code/include/989snd_iop.h` as
  `SND_IOP_CMD_CD_BANK_LOAD_BY_LOC` (Deadlocked `_CB` twin issues opcode 3 with
  8-byte params for the same operation).

## Dead tail func_0012E078

0x12E078: `addiu $sp,$sp,0x60; nop; addiu $sp,$sp,0x50` (+ alignment nop) —
unreachable fragment the original compiler emitted after the epilogue
(ECG 2.95.2 never regenerates it; sizes match no frame). Preserved as raw asm
in the new TU, per the dead-tail ghost policy (not a blocker).

## Probes

`working/989snd_snd_BankLoadByLoc/` (cleared after commit): p4 GNU plain extern
(0x144, 41 diffs — established GPREL-vs-absolute map), p5 GNU two-name (link
fail: req/server lost `.data`), p6 SN plain+Gp (24 diffs — bne tail shape),
p7 volatile Gp + do-while (0x15C, direction flip), p8 non-volatile + do-while
(0x154, **1 diff**: move vs reload), p9 + GpRet alias (**0 diffs**).
