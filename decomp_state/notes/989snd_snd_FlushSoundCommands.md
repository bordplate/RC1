# snd_FlushSoundCommands (code/989snd/ee/989snd.c) - BLOCKED 2026-09-17

`int snd_FlushSoundCommands(void)` at vram 0x12DC80, 0x1E0 bytes (480 bytes / 120 words).
C-linkage (unmangled). `989snd.c` is one of the five GNU-compatibility TUs, so the
candidate builds with `ASSEMBLER=gnu` (`-Wa,-EL -Wa,-Icode/include`) and the common
flags `-G8 -O2 -ffast-math -fno-exceptions`. gp = 0x166C00.

## What the function does
Returns nonzero if the EE sound system still has pending work. Four blocks:
1. **Return-callback / stream-entry flush.** If `snd_currentBuffer` (0x15EC80) is set and
   `snd_GotReturns()` is true: if `snd_returnCallbackPending` (0x15EC9C), invoke
   `snd_returnCallbackFn(snd_iopReturnWord, snd_returnCallbackData)` (0x133104) and clear
   it; else iterate the active batch's stream-entry table (`snd_batchIndex != 1` selects the
   buffer pair) and call each pending `entry->fn`.
2. **CD-callback flush.** If `snd_cdCallbackPending` (0x15ECC8), `FlushCache(0)`; if
   `snd_cdCallbackArg` (0x15ED00) is not the `0xFFFFFFFF` sentinel, and the callback
   `snd_cdCallbackFn` (0x15ECD0) is set, invoke it with the arg and the 64-bit
   `snd_cdCallbackData` (0x15ECD8), then clear the arg/data/fn/pending globals.
3. **Send batch.** If `snd_currentBuffer` is clear and the current command buffer is
   nonempty and `snd_batchBusy` (0x15ECC4) is clear, `snd_SendCurrentBatch()`.
4. **CD sync.** If `snd_cdSyncPending`, `snd_StreamSafeCdSync(1)`; if
   `snd_cdStreamEndPending` clear it and, if `snd_cdStatusCallback` is set, clear that and
   invoke the status callback.

Returns `(snd_currentBuffer != 0) || (snd_cdCallbackPending != 0)`.

## Match status
Blocks 1, 3, 4 and the epilogue match byte-for-byte in the best candidate. The stream-entry
for-loop matches except one `addu` operand order at 0x12dd1c. The **CD-callback block
(0x12dd58-0x12ddb8) does not match** — it is a tight cluster of EGC register-allocation and
scheduling decisions that no C form reproduced. Best candidate: `flush_v10.c` (476 bytes vs
480; 60 word diffs, all in the CD block + the one addu).

## CD-callback block ground truth (objdump of assets/boot_elf.elf)
```
12dd58: lw   v0,-32568(gp)      # K = snd_cdCallbackPending (0x15ECC8)
12dd5c: beqz v0,0x12ddb8         # K==0 -> bnez A (skip)
12dd60: lw   v0,-32640(gp)      # A = snd_currentBuffer [delay]
12dd64: jal  FlushCache
12dd68: move a0,zero             # [delay]
12dd6c: lui  v0,0xffff
12dd70: lui  a0,0x16
12dd74: lw   a0,-4864(a0)        # a0 = arg (0x15ED00) SELF-BASED
12dd78: ori  v0,v0,0xffff         # v0 = 0xFFFFFFFF
12dd7c: beq  a0,v0,0x12ddb4      # arg==-1 -> lw A   [NON-likely]
12dd80: lw   v0,-32560(gp)       # v0 = fn (0x15ECD0)   [FN IN v0]
12dd84: beqzl v0,0x12ddb0        # fn==0 -> K=0       [LIKELY, taken when fn==0]
12dd88: sw   zero,-32512(gp)     # arg=0 GPREL [delay of 12dd84 = NON-NULL path]
12dd8c: lui  a1,0x16
12dd90: ld   a1,-4904(a1)        # a1 = data (0x15ECD8) SELF-BASED
12dd94: sw   zero,-32560(gp)     # fn = 0
12dd98: lui  at,0x16
12dd9c: sd   zero,-4904(at)      # data = 0 SELF-BASED  [BEFORE call]
12dda0: jalr v0                   # fn(arg,data)         [delay = nop]
12dda4: nop
12dda8: lui  at,0x16
12ddac: sw   zero,-4864(at)       # arg = 0 SELF-BASED   [AFTER call]
12ddb0: sw   zero,-32568(gp)      # K = 0
12ddb4: lw   v0,-32640(gp)        # A
12ddb8: bnez v0,0x12ddf4
```
Note the `beqzl` at 12dd84 is a likely branch: its delay slot (arg=0, 12dd88) is annulled
when taken (fn==0) and EXECUTES on the non-null path (fn!=0). So arg=0 is a non-null-path
store (appears twice on that path: 12dd88 delay + 12ddac tail), and K=0 is reached by both
the fn==0 and fn!=0 paths but not the arg==-1 path. The non-null path order is:
`arg=0; data-load; fn=0; data=0; call; arg=0; K=0`, with the data/arg stores SELF-BASED
(fresh `lui` per access, base reg == dest/temp) and the data clear BEFORE the call.

## The remaining (unreproducible) differences
1. **fn register + call target.** The original loads fn into `v0` (12dd80) and reuses that
   same `v0` for `jalr v0` (12dda0) across the `fn=0` clear (12dd94). Calling through the
   global after clearing it makes local EGC re-load the cleared 0 (`li at,0; jalr at`, a
   null call — v9). So the call must go through a local snapshot; but the snapshot lands in
   `a2` (v6) or `v1` (v5), not `v0`. Pinning `register void (*cb)() asm("$2")` (v10) does
   put fn in `v0` and yields `jalr v0`, but the rest of the block still differs.
2. **`beqzl` likely vs `beqz` non-likely** for the `fn != 0` test. EGC emits the non-likely
   form in every candidate; no C form produced the likely branch.
3. **Self-based vs split `.data` addressing.** The original re-materializes a fresh `lui`
   per access (base == dest). EGC hoists the `lui 0x16` into a shared base register (v1/a2)
   for the data load and the data/arg clears (split form), even with distinct `.data` alias
   symbols for the clears (v5/v10). `-mno-split-addresses` does NOT fix it here — it shifts
   the whole function and makes it worse (v10nosp: 488 bytes, 111 diffs).
4. **Store ordering across the call.** The original hoists the data load + data clear + fn
   clear BEFORE the call (call delay = nop); EGC packs the data load into the call's delay
   slot and leaves the clears after the call (v5/v10).
5. **addu operand order at 0x12dd1c** (stream-entry loop): original `addu a0,v0,v1`
   (offset,base) vs candidate `addu a0,v1,v0` (base,offset). A standalone `buf[idx]+i` probe
   does emit the original order, so this is full-function RA context; `i + (T*)base` (the
   last-resort suggestion) was not separable from the CD-block mismatch.

## Attempts (all via tools/decomp_probe.py, /tmp/opencode/flush_v*)
- v1 `[argGp=0; call; fn=0; data=0; arg=0]` all `.data` names -> split bases, stores after call.
- v2 clears before call calling THROUGH THE GLOBAL -> broken null call (`jalr at`, at=0).
- v3 bare `for` loop (no `if(count>0)` wrapper) -> fixed the whole loop region.
- v4 `arg != 0xFFFFFFFF` (not `!= -1`) -> fixed the -1 constant (`lui/ori` split).
- v5 distinct `.data` clear aliases (argGp GPREL + argClear/dataClear `.data`) -> 476 B, 61 diffs.
- v6 snapshot locals (callback/data), clears before call, call through local -> 476 B, fn in a2.
- v7 common-tail `arg=0` (GPT-6 Astra expert's form, aliases removed) -> 460 B (over-optimized), worse.
- v8 v7 + `register` pins `$4/$2/$5` -> 460 B, still over-optimized.
- v9 snapshot only `data`, call through global after clearing fn -> null call (like v2).
- v10 v6 + `register void (*callback)() asm("$2")` -> 476 B, 60 diffs; fn now in v0, `jalr v0`
  correct, but beqz (non-likely), split data/arg bases, data-clear after call, argGp not in
  the delay slot. Best so far (kept at decomp_state/probes/flush_v10.c).
- v10nosp: v10 + `-mno-split-addresses` -> 488 B, 111 diffs (flag makes it worse; the
  self-based accesses are NOT a flag issue in this function).
- kprobes (bare `for` loop, K-hoisting, `!= 0xFFFFFFFF` constant form) confirmed the loop and
  constant findings above.

## Escalations
- **expert (GPT-6 Astra)** invoked 2026-09-17: recommended the snapshot-into-locals /
  clear-before-call / common-tail form (v7). Tested: over-optimized to 460 B (worse). It also
  claimed the 12dd88 delay-slot store is on the NULL path ("annulled when non-null") — that
  is inverted; a MIPS likely branch annuls the delay slot when TAKEN, so `beqzl v0,0x12ddb0`
  (taken when fn==0) executes 12dd88 on the NON-NULL path.
- **last-resort-decompiler (GPT-5.6 Sol)** invoked 2026-09-17: recommended (1) the alias-free
  common-tail form (v7 — tested, worse), (2) `register asm("$4/$2/$5")` pins (v8 — tested,
  worse with the common-tail base; the `$2` pin alone, v10, does fix the fn register),
  (3) a zero-byte memory barrier after the data clear, (4) moving the callback snapshot before
  the arg test, (5) `i + (T*)base` for the addu. Its "key correction" about the delay-slot
  path is the same inverted likely-branch claim as the expert. No recommendation produced a
  full match; the register pin (v10) is the only net gain.

## Blocker
EGC 2.95.2 (project flags) cannot be steered, by any C form or block-local flag, to
reproduce the CD-callback block's combined requirements: fn loaded into `v0` and reused for
`jalr v0` across the `fn=0` clear, a `beqzl` (likely) fn test, self-based (fresh-`lui`,
base==dest) `.data` accesses for the arg/data load and clears, the data clear scheduled
BEFORE the call with a nop call delay slot, and the arg=0 store duplicated into the `beqzl`
delay slot. Each requirement is an independent RA/scheduling decision; the register pin fixes
only the fn register (v10) and `-mno-split-addresses` makes the function worse. The remaining
diffs are 60 words (476 vs 480 bytes), all in 0x12dd58-0x12ddb8 plus the 0x12dd1c `addu`.
INCLUDE_ASM retained. Revisit if the EGC build/flags are revisited or a per-access
self-based-addressing control is added to the toolchain.

## The dead tail (func_0012DE60) — blocked, INCLUDE_ASM retained

The 0xC bytes at 0x12DE60 (file 0x2EDE0), immediately after this function's
`jr ra; addiu sp,sp,0x50` epilogue and before `snd_GotReturns` (0x12DE70):

```
0x12DE58: jr   ra
0x12DE5C:      addiu sp, sp, 0x50    (delay slot; complete restore)
0x12DE60: addiu sp, sp, 0x10         <- unreachable dead tail (Splat symbol func_0012DE60)
0x12DE64: nop
0x12DE68: addiu sp, sp, 0x10         <- unreachable
0x12DE6C: nop                        (alignment padding after endlabel)
```

- A 2x0x10 dead deallocate fragment, not a "duplicate of the parent's 0x50
  dealloc" (unlike the single-0x10 sibling func_0012EC00). The parent's own
  frame is fully restored in the `jr` delay slot, so these two adds are pure
  dead code. Same-TU two-deallocate tails also occur at func_0012E270,
  func_0012E198, and func_0012EF58.
- Deadness: Ghidra has no function at 0x12DE60 and no xrefs to it; a raw-ELF
  scan finds 0 jal and 0 j instructions targeting 0x12DE60 (jal word
  0x3004B798 and j word 0x2004B798 are absent from the entire boot ELF); the
  parent's branches (all targets 0x12DD18-0x12DE44) do not reach it.
- Not regenerable: local EGC 2.95.2 v2.73a (`-G8 -O2 -ffast-math
  -fno-exceptions -snas`) probes (deadaddiu.c, t1/t6) emit exactly ONE
  reachable `addiu sp,sp,N` in the epilogue and never a dead one after `jr ra`
  (a 0x40 frame emits a single `addiu sp,sp,0x40`, not split into adds). This
  matches the func_0012EC00 t1-t10 experiments: EGC's only dead-tail mechanism
  is the dead *store*, never a dead frame deallocation. Scheduler flags reorder
  existing RTL but cannot synthesize a missing post-return epilogue.
- A standalone C function cannot match: it has no prologue, no `jr ra`/return,
  and would fall through into snd_GotReturns after corrupting sp.
- **last-resort GPT-5.6 Sol** invoked 2026-09-17: confirmed no credible C form or
  TU flag reproduces the fragment with the current toolchain; recommended
  retaining the orphan INCLUDE_ASM and blocking the entry.
- Therefore the orphan `INCLUDE_ASM(..., func_0012DE60)` is retained to supply the
  bytes (preserving full boot-ELF parity), and the queue entry is blocked
  (precedent: func_0012EC00, func_001FDD50, func_00233880).
