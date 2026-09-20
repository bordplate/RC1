# snd_SendIOPCommandNoWait (0x12E6E0, 176 words)

Blocked 2026-09-20. The C body is fully understood and was implemented; the
local EGC (SN 2.73a) cannot reproduce the original register allocation.
INCLUDE_ASM retained.

## Function

Sends a command to the IOP 989 sound server. Fast path: when
`snd_batchBusy == 0 && snd_currentBuffer == 0 && data_size == 0 &&
done == 0`, the command is issued synchronously as a fire-and-forget
`sceSifCallRpc(&snd_rpcServer, command, 1, 0, 0, snd_syncBuffer, 0xC, 0, 0)`
after `snd_PrepareReturnBuffer(snd_syncBuffer, 1)`, spinning on
`sceSifCheckStatRpc` (with `func_00116078(&snd_NonIdleErrorString);
snd_FlushSoundCommands(); FlushCache(0);`) while the RPC is non-idle.
Batch path: otherwise it spins until the current command batch
(`snd_batchCommandBuffers[snd_batchIndex]`) has at least `aligned =
round_up_4(data_size + 4)` free bytes, appending the 4-byte header
(`command`, `data_size`) plus the payload, updating
`snd_streamBuffers[idx][num_commands].done/.u_data`, and calling
`snd_PostMessage()` which bumps `num_commands` and flushes. `was_cleared`
tracks that `snd_batchBusy` was cleared during the spin and re-sets it
afterwards. Progress/error reporting via `func_00116078`
(`snd_NoWaitBufferFullString` 0x154010 when the batch is full on the second
spin, `snd_NoWaitContinuingString` 0x154070 with the spin count afterwards).

## Verified facts

- Signature: `void snd_SendIOPCommandNoWait(int command, int data_size,
  char* data, SndCompleteProc done, u64 u_data)`; args in a0..a4 where
  u_data is a SINGLE 64-bit register (a4/t0): original callers set only
  `move t0, ...` and the callee spills it with one `sd t0, 16(sp)`.
- `SndCommandReturnDef { SndCompleteProc done; u64 u_data; }` (16 bytes).
- `SndCommandBuffer { int num_commands; char buffer[4092]; }`.
- `snd_batchCommandBuffers` is `SndCommandBuffer*[2]` (0x15ECA0).
- The five forwarding wrappers prove the typed params: 3-arg wrappers
  (0x19/0x32/0x4F) forward `b` with `move a3,a1` and `c` with
  `move t0,a2` (full 64-bit); 8-arg wrappers (0x11/0x21) forward from
  t2/t3 per the EGC 7th/8th-arg ABI.
- Deadlocked (reference/dl/989snd/ee/989snd.c) has the same function with
  locals `x` (copy index, reg a2), `was_caching`, `y` (retry count, reg
  s2), `msg` (pointer, reg a1), `msg_size` (aligned size, reg s0) — the
  register assignments match the RC1 original binary exactly.

## The blocker: register allocation

Original (frame 0xC0, u_data at 0x10(sp)) keeps ALL 9 s-registers for real
values: s0=aligned, s1=data_size, s2=i, s3=&cmdBufs, s4=&freeBytes,
s5=was_cleared, s6=data, s7=command, s8=done. The spin-loop constants stay
in caller-saved registers and are RELOADED in gaps: 256 as `li a1,256`
(loop entry 0x12E7FC, reloaded 0x12E86C after the printf), 1 as `li v1,1`
placed AFTER the flush call (0x12E83C), string base `lui a0,0x15` inside
the i==1 branch only (0x12E84C).

Local EGC (frame 0xD0) hoists the constant materializations to the TOP of
the loop body (before `snd_FlushSoundCommands`), so they are live across
the call and get call-safe s-registers: s4=256, s5=strBase, s6=1 — which
steals the s-registers from the incoming params, so command/data/done are
SPILLED (`sw a0,16(sp); sw a2,20(sp); sw a3,24(sp)` in the prologue,
reloaded at each use). This flips every comparison/store in the batch
path. The control flow, the fast-path layout (including the stale-v0
`currentBuffer` quirk at 0x12E7C0), the tail `li -1; slt; movn` idiom and
the copy loop all match when written as:

- `was_cleared = 0;` at the top (prologue `move s5,zero`)
- tail as a TERNARY `tail = (aligned > -1) ? aligned : data_size + 7;`
  (the if-form `if (-1 < aligned) tail = aligned;` folds to the wrong
  `slti; movz`)
- `if (was_cleared) snd_batchBusy = 1;` (NOT `||`)
- bare copy loop `for (j = 0; j < data_size; j++) p[j] = data[j];`
  (the explicit `if (data_size > 0)` wrapper emits a second `blez`)

## Attempts (word-diff vs original 176 words; 0 = match)

- baseline if/else 178; no-else + post-guard aligned= 151; combined guard
  151; for(;;)+break 157
- decl-order permutations 151; `was_cleared = 0` init 151; `i = 0` init 178;
  guard operand order 152; `long` for u_data 151
- flags: -fno-schedule-insns 174, -fno-schedule-insns2 178, -O1 181,
  combos 178, -G0 197, -fno-move-all-movables 151,
  -fno-strength-reduce 151, -fno-gcse 163,
  -fno-expensive-optimizations 151, -fomit-frame-pointer 151,
  -fno-caller-saves 151 (all accepted by EGC)
- Deadlocked debug-symbol-shaped source (locals x/was_caching/y/msg/
  msg_size, natural `if (msg_size % 4) msg_size += 4 - (msg_size % 4);`)
  171 (else form) / 149 (no-else microvariant) — same allocation
- `register X asm("$23"/"$22"/"$30")` pinning of command/data/done: 164
  and appears to miscompile (`addiu s1,s8,4` = done+4)

Root cause: an allocator tie-break difference between the local EGC (SN
2.73a) and Insomniac's SCE 2.95.2 at 9+ s-register demand — local EGC
ranks compiler-generated values/loop constants above incoming params for
s-registers; the original fills s-registers params-first. No matched
function in the project saves 7+ s-registers (max = 6), so this scale was
never before validated.

## Escalations

- `expert` (GPT-6 Astra, 2026-09-20): could not read the dossier (prompt
  expansion bug) but recommended pass flags (-fno-move-all-movables,
  -fno-strength-reduce, -fno-gcse, -fno-expensive-optimizations,
  -fomit-frame-pointer, -fno-caller-saves) and a register-variable
  diagnostic — all tested above, none matched.
- `last-resort-decompiler` (GPT-5.6 Sol, 2026-09-20): recommended the
  debug-symbol-shaped source with the natural modulo idiom (tested: 171/
  149) and stated that if the no-else microvariant retains the
  constant-hoisting allocation, blocking is justified. It does (149).

## Kept in the tree (parity-verified)

- Correct `SndCompleteProc`/`SndCommandReturnDef`/`SndCommandBuffer`
  typedefs and the typed NoWait extern in 989snd_post.c / 989snd_pre.c
- `func_00116078` made variadic in 989snd_post.c (verified codegen-
  identical: no hidden va_list arg for 1-3 arg calls; 989snd_pre.c was
  already variadic)
- `snd_batchCommandBuffers` typed `SndCommandBuffer*[2]` with `->
  num_commands` at all use sites (matched snd_SendIOPCommandAndWait,
  snd_PostMessage, snd_SendCurrentBatch re-verified by full parity)
- Five wrapper signatures retyped to `(…, SndCompleteProc, u64)`; `(char*)`
  casts added at the 18 int-buffer call sites (Deadlocked style; casts are
  codegen no-ops)
- New symbols: snd_NoWaitBufferFullString = 0x00154010,
  snd_NoWaitContinuingString = 0x00154070 (symbols.txt +
  linker_aliases.ld); generated INCLUDE_ASM now references the names
