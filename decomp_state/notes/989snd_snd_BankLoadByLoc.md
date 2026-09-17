# snd_BankLoadByLoc (0x12DF20, 340 bytes) — BLOCKED

Sound-bank-load RPC wrapper. Sets up a request struct, calls the 9-arg
`sceSifCallRpc`, then waits for a callback-arg global at **0x15ED00** (inside the
gp window, gp=0x166C00) to stop being the sentinel 0xFFFFFFFF.

Best candidate reached: **20 word diffs, 336 vs 340 bytes, correct frame (0x50)**.
All 20 diffs are confined to the RPC-arg setup (0x12dfd4-12dff0) and one
scheduling `nop` (0x12e050) that cascades the downstream branch offsets + epilogue.

## The 5 access forms of 0x15ED00 in the original
1. Absolute store (0x12df90-94): `lui at,0x16; sw v1,-4864(at)`, value `lui v1,0xffff; ori v1,v1,0xffff` (0x12df80-88).
2. RPC 6th arg (0x12dfec-f0): `lui t1,0x16; addiu t1,t1,-4864` — register load, SIGNED split, ADJACENT at call site, NOT hoisted.
3. GPREL load (0x12e004, bgez delay): `lw v1,-32512(gp)` — if-check.
4. GPREL load (0x12e030, bne delay): `lw v0,-32512(gp)` — EARLY-RETURN value (a second, fresh GPREL load, NOT CSE'd with #3).
5. Absolute load (0x12e048-4c): `lui v0,0x16; lw v0,-4864(v0)` — do-while poll.

Key control-flow fact (expert insight): the early success exit is NOT `return -1`.
`bne v1,v0,epilogue` [delay: `lw v0,-32512(gp)`] — when taken, `v0` (the return
value) is set by a SECOND fresh GPREL load in the delay slot. Modeling it as
`return -1` makes EGC constant-propagate that load to `li -1`; modeling it as
`return <fresh load>` makes it a real GPREL load (matches 0x12e030). The loop is a
do-while (back test only, no front-test branch).

## Best candidate (v12) C shape
```
snd_cdLoadError = 0;
if (snd_cdCallbackPending) { report(InProgress); return 0; }
if (snd_StreamSafeCdSync(1) == 1) { report(CDBusy); return 0; }
request.field_04 = a1;
*(unsigned int*)0x15ED00 = 0xFFFFFFFF;      // constant store
request.src = a0;
while (sceSifCheckStatRpc(&snd_cdRpcServer) != 0) { report(NonIdle); snd_FlushSoundCommands(); FlushCache(0); }
result = sceSifCallRpc(&snd_cdRpcServer, 3, 1, &request, 8, (void*)0x15ED00, 4, 0, 0);  // constant RPC arg
if (result < 0) { report(Rpc); snd_cdLoadError = 0x106; return 0; }
if (g1 != 0xFFFFFFFF) return g2;            // g1,g2 = plain extern int @0x15ED00 (GPREL)
do { FlushCache(0); arg = *(int*)0x15ED00; } while (arg == 0xFFFFFFFF);
return arg;
```
This matches everything EXCEPT the RPC-arg split/hoisting and the 0x12e050 nop.

## The two unresolved issues

### 1. RPC 6th arg: signed split that stays at the call site (UNSOLVABLE)
- **Constant** `(void*)0x15ED00`: EGC lowers it through `li` → UNSIGNED split
  (`lui t1,0x15; ori t1,t1,0xED00`) and hoists the `lui` to 0x12dfd4. Right frame,
  wrong split + wrong position.
- **`.data` symbol** `&sym`: EGC uses the SIGNED split (`lui t1,0x16; addiu
  t1,t1,-4864`) BUT the loop optimizer LICM's the invariant `%hi(sym)` into the
  stat-loop preheader; it survives the calls, so register allocation gives it a
  callee-saved register → **frame grows to 0x60** (s3 saved). Reproduced with 1, 2,
  4 separate same-address symbols, a volatile symbol, and an array-decay form.
- No declaration qualifier, cast, struct indirection, array decay, `register`
  asm("$9")+barrier, or flag yields signed + unhoisted + frame 0x50 together.
  `volatile` applies to accesses, not address computation.

### 2. The `nop` at 0x12e050 (scheduling artifact)
Original: `lw v0,-4864(v0); nop; beq v0,s0,loop; nop`. The candidate packs the
`beq` up to 0x12e050 (missing the pre-beq nop). `asm volatile("nop" : : "r"(arg))`
emits a real nop but the assembler's `.set reorder` moves it into the beq DELAY
slot, not the pre-beq position — so it does not fix 0x12e050.

## Flags tested (all EGC 2.95.2, -G8 -O2 -ffast-math -fno-exceptions, GNU asm)
- `-mno-split-addresses`: breaks the whole function (352B/78 diffs) — perturbs every
  other symbolic address (matches AGENTS.md warning).
- `-fno-schedule-insns`: 26 diffs, regresses store + error path.
- `-fno-strength-reduce` (on symbolic RPC arg): 71 diffs, frame 0x60 (does not undo
  the pre-scheduling LICM).

## Escalation
- **expert (GPT-6 Astra)**: identified the early-return-as-fresh-load (fixed the
  snapshot); recommended `.data` symbols + `-mno-split-addresses` (failed) and
  scheduling isolation.
- **last-resort GPT-5.6 Sol used**: recommended (A) array-decay RPC arg, (B)
  `register void* asm("$9")` + zero-byte barrier, (C) `-fno-strength-reduce` /
  `-fno-rerun-loop-opt` / `-fno-gcse`, and a bound `asm volatile("nop")`. All tested:
  A → 71 diffs/frame 0x60; B → C89 parse error (register-var init); C → 71 diffs/
  frame 0x60; the bound nop → moved into the beq delay slot. None closes the diffs.

## Conclusion / policy
The only near-matching form (v12) depends on **static data-address constants**
(store, RPC pointer, poll), which are forbidden in source (overlay compatibility),
and it still has 20 diffs. Every symbolic form is blocked by EGC's pre-scheduling
address LICM (frame growth). The function is NOT representable as an acceptable
symbol-based EGC 2.95.2 translation under the available per-TU flags. Kept as
`INCLUDE_ASM`.

Newly named symbols (kept, they are real globals): `snd_cdBankLoadRequest`
(0x137B40), `snd_cdLoadError` (0x15EC88), `snd_BankLoadByLoc{InProgress,CDBusy,
Rpc}ErrorString` (0x153DA8/0x153DD8/0x153DF8). Probe-only same-address aliases were
removed.
