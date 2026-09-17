# snd_GotReturns

## Objective

Match `snd_GotReturns` at 0x12DE70 (0x88 bytes, `code/989snd/ee/989snd.c`).
First active queue target after refactor.json emptied.

## Semantics

Polls the IOP return buffer armed by `snd_PrepareReturnBuffer`
(`snd_currentBuffer` / `snd_currentBufferIndex`, 0x15EC80 / 0x15EC84):

1. `FlushCache(0);`
2. `snd_currentBuffer == 0` -> `return 1` (nothing armed).
3. `sceSifCheckStatRpc(&snd_rpcServer) != 0` (RPC still busy) -> `return 0`.
4. `*buf == 0xFFFFFFFF && buf[index+1] == *buf` ("no return yet" sentinel in
   both slots) -> `snd_currentBuffer = 0; return 1` (disarm).
5. Unexpected slot contents -> `func_00116078(&snd_NoReturnErrorString);
   return 0;` where 0x153C98 is the rdata string
   `"989snd.c: Sif says RPC isn't busy, but we still don't have return from
   IOP!\n"` (new symbol `snd_NoReturnErrorString`).

Callers poll on the zero result: 0x12E668 (`jal; ...; beqz v0, loop`) and
0x12DCA8 (`jal; beqz v0, 0x12DD58`). The return value is consumed, so the
function is `int`.

## Findings

- The `daddu $2, $0, $0` in the `bnez` delay slot at 0x12DE9C (Splat
  misdecode of the zero-move) is a LIVE return value: the stat-busy path
  returns 0, not the stat. Modeling it as `return stat;` mis-schedules the
  epilogue (v1 probe).
- The store at 0x12DED4 is `sw zero, -0x7F80(gp)`: a GPREL store to the
  `snd_currentBuffer` global itself (pointer disarmed to 0), NOT
  `*snd_currentBuffer = 0` (that form fails, v4 probe).
- EGC constant-split quirk: `!= -1` emits a single `addiu v0, zero, -1`;
  the original uses `lui v0, 0xffff; ori v0, v0, 0xffff`. Only the unsigned
  spelling reproduces it: `0xFFFFFFFF`, `(unsigned)-1`, and `~0u` all match;
  `(int)0xFFFFFFFFL` fails (22 word diffs). The final source names the
  value `SND_RETURN_SLOT_NONE` (preprocessor define, codegen-neutral).
- Two separate `return 1;` sites stay separate in EGC output when guarded by
  different conditions (each `b` + delay-slot `li`), matching the original;
  merging them into one path (v1/v2 probes) mis-schedules the stores.
- MISNOME found while wiring the error string: `snd_batchCommand`
  (0x153D20) was actually the rdata string
  `"989snd.c: RPC still nonidle!\n"` passed to the error reporter
  `func_00116078` (used by `snd_SendCurrentBatch` and the four nonmatching
  bank-load/IOP-command functions). Renamed to `snd_NonIdleErrorString` in
  `config/symbols.txt`, `config/linker_aliases.ld`, and all references
  (codegen-neutral: same address).
- `snd_currentBuffer` changed from `extern void*` to `extern int*` so the
  new C can subscript it; codegen-neutral for the only other user, matched
  `snd_PrepareReturnBuffer` (parity confirmed).
- `func_00116078` (0x116078) is the SDK error-reporting routine taking a
  message string; retained address-based name per the
  `snd_SendCurrentBatch` precedent.
- Both error strings sit OUT of the GP window, so the C externs use
  `__attribute__((section(".data")))`.

## Verification

- `tools/decomp_probe.py` with `--assembler gnu` (989snd.o is a GNU-TU):
  136/136 bytes, 0 diffs (probes `decomp_state/probes/gotret_v1..v5.c`).
- `make split` + full rebuild: `cmp build/boot_elf.elf assets/boot_elf.elf`
  passes; direct 0x88-byte slice at 0x12DE70 identical to the original.
- `tools/decomp_status.py --count`: 689 -> 688.

## Resolution

Matched 2026-09-17. Final form in `code/989snd/ee/989snd.c` (function at
line 33) with the `SND_RETURN_SLOT_NONE` define; externs for
`FlushCache`, `snd_rpcServer`, `sceSifCheckStatRpc`, `func_00116078`, and
`snd_NoReturnErrorString` moved above the function (C89 declaration order);
the later extern group keeps `snd_NonIdleErrorString` and the batch globals.
