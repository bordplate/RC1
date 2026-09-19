# snd_FlushSoundCommands (code/989snd/ee/989snd_flush.c) - MATCHED 2026-09-19

`int snd_FlushSoundCommands(void)` at vram 0x12DC80, 0x1E0 bytes (480 bytes / 120 words).
C-linkage (unmangled). The function lives in `989snd_flush.c`, an SN-as TU (default
`-snas`); the sibling `989snd_pre.c` (snd_StartSoundSystem) and `989snd_mid.c`
(0x12DE60+) keep GNU-as. The match is SN-as-specific (see below). gp = 0x166C00.

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

## Match
All four blocks + the epilogue match byte-for-byte (object 120/120 words, full `make` +
`cmp build/boot_elf.elf assets/boot_elf.elf` pass). The CD-callback block
(0x12DD58-0x12DDB8) was the last blocker and is solved by three things:

1. **Late `fn` capture.** Snapshot `fn = snd_cdCallbackFn` *after* the `0xFFFFFFFF`
   sentinel test (nested `if`), not before. The sentinel value is left in `v0` by the
   test; capturing `fn` after it lets the allocator reuse `v0` (the sentinel's dead
   register) for `fn`, so `jalr v0` reuses that same `v0` across the `fn=0` clear.
   Capturing `fn` before the sentinel test (the comma/early form) forces `fn` into `v1`
   and breaks the `jalr v0`.
2. **Two zero-instruction memory barriers.** `asm volatile("" : : : "memory");`
   (emits no machine instruction, only `#APP`/`#NO_APP`). One at the top of the
   `if (fn != 0)` block pins the data load out of the `fn`-test delay slot, which (a) lets
   the `arg=0` clear take that delay slot — producing the `beqzl` (likely) `fn` test and the
   duplicated `arg=0` store — and (b) forces the data load to the self-based two-instruction
   form in the macro region. One between `snd_cdCallbackData = 0` and the call pins the data
   clear before the call, leaving the call delay slot a `nop`.
3. **`.extern` seeds + SN-as.** The SN single-pass assembler expands a bare `lw/sw r,sym`
   pseudo GPREL only if a `.extern sym,N` appeared earlier in the file; the flush TU seeds
   the small-data globals up front (arg/data deliberately unseeded so they expand
   self-based). The match is SN-as-specific: the identical C under GNU-as is 116 words with
   many diffs, so the `989snd_pre` Splat segment was split into `989snd_pre` (GNU-as) +
   `989snd_flush` (SN-as, this fn) + `989snd_mid` (GNU-as, 0x12DE60+).

The `snd_FlushSoundCommands` prototype was corrected `void` -> `int` in `989snd_post.c` and
`989snd_bankload.c` (all callers discard the return, so the change is codegen-neutral).

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
The `beqzl` at 12dd84 is a likely branch: its delay slot (arg=0, 12dd88) is annulled when
taken (fn==0) and EXECUTES on the non-null path (fn!=0). So arg=0 is a non-null-path store
(appears twice on that path: 12dd88 delay + 12ddac tail), and K=0 is reached by both the
fn==0 and fn!=0 paths but not the arg==-1 path. The non-null path order is:
`arg=0; data-load; fn=0; data=0; call; arg=0; K=0`, with the data/arg stores SELF-BASED
(fresh `lui` per access, base reg == dest/temp) and the data clear BEFORE the call.

## Requirements the solution satisfies
1. **fn register + call target.** Late capture puts `fn` in `v0` (the sentinel's dead
   register), reused for `jalr v0` across the `fn=0` clear.
2. **`beqzl` likely vs `beqz` non-likely.** The `arg=0` store in the `fn`-test delay slot
   (from barrier 1) is what makes EGC emit the likely form.
3. **Self-based vs split `.data` addressing.** The unseeded arg/data + the barriers keep the
   data load/clear self-based (fresh `lui` per access) instead of a shared hoisted base.
4. **Store ordering across the call.** Barrier 2 keeps the data load + clears before the
   call (call delay = `nop`).
5. **addu operand order at 0x12dd1c** (stream-entry loop) resolved by the full-function RA
   context once the CD block matched.

## Historical attempts (pre-match, via tools/decomp_probe.py)
- v1-v6: various snapshot/alias/loop forms; best was v6 (fn in a2, split bases).
- v7: common-tail `arg=0` (GPT-6 Astra expert's form) -> over-optimized to 460 B (worse).
- v8: v7 + `register` pins `$4/$2/$5` -> 460 B, still over-optimized.
- v10: v6 + `register void (*callback)() asm("$2")` -> 476 B, 60 diffs; fn in `v0` and
  `jalr v0` correct, but beqz (non-likely), split bases, data-clear after call. Best of the
  pre-match set (kept at decomp_state/probes/flush_v10.c).
- v10nosp: v10 + `-mno-split-addresses` -> 488 B, 111 diffs (flag makes it worse).
- The decisive step was the late `fn` capture + the two memory barriers (see Match), which no
  pre-match variant tried.

## Escalations
- **expert (GPT-6 Astra)** and **last-resort-decompiler (GPT-5.6 Sol)** were each invoked
  2026-09-17 on this target; neither produced a match (their forms tested as v7/v8, worse).
- The late-`fn`-capture hypothesis (the sentinel leaves `v0` live for `fn`) plus the
  delay-slot pinning barriers closed the block.

## Verification
- Object: 120/120 words identical to the original (reloc-aware word diff).
- Full build: `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` byte-for-byte.
- The stale `blocked.json` entry was removed; the matched entry was added.

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
- Therefore the orphan `INCLUDE_ASM(..., func_0012DE60)` is retained (now in
  `989snd_mid.c`) to supply the bytes (preserving full boot-ELF parity), and the
  queue entry is blocked (precedent: func_0012EC00, func_001FDD50, func_00233880).
