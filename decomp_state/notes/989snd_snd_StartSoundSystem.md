# 989snd_snd_StartSoundSystem (VMA 0x12DA28, 0x258 / 600 bytes)

Target of one full attempt. Structurally understood and a C candidate compiles,
but it does NOT match byte-for-byte: EGC's register allocation and prologue
scheduling differ from the original. Reverted to INCLUDE_ASM (parity intact).

## Verified facts

- Final-ELF VMA is **0x12DA28** (objdump/section headers; the generated `.s`
  comment `0012DA28` is CORRECT). Raw bytes sit at file offset 0x2E9A8.
  (An earlier revision of this note claimed 0x11EA28 from the PT_LOAD
  `vma=fileoff+0xF0080` mapping — that is the program-header address, NOT the
  section-header VMA the build/cmp use. `objdump -d` places the prologue at
  0x12DA28 and a different function at 0x11EA28; trust objdump.)
- Frame 0xB0; saves s0-s8 + ra (10 `sq`). Locals at 0(sp) (`data`).
- It is the sound-system startup: sets the 6 batch/stream/return buffer pointer
  slots, inits the SIF, binds two RPC servers with a retry+spin, clears the CD
  callback state, and issues a first IOP command.

## Data layout (all verified against objdump)
- 0x15ECA0/4: batchCommandBuffers[0/1] = &cmdBuf1(0x133280)/&cmdBuf2(0x134280)
- 0x15ECB8/0x15ECBC: batchReturnBuffers[0/1] = &retBuf1(0x137280)/&retBuf2(0x1376C0)
- 0x15ECB0/0x15ECB4: streamBuffers[0/1] = &strBuf1(0x135280)/&strBuf2(0x136280)
- 0x15EBC0: rpcServer; 0x15EBE8: cdRpcServer. `.bound` flag at offset 0x24
  (rpcServer.bound=0x15EBE4, cdRpcServer.bound=0x15EC0C). Server block layout
  (from bind FUN_0011aff8): [0]=blk*, [1]=int, [2]=sema, [4]=0, [9]=bound.
- 0x15ECC8/0x15ECD0/0x15ECD8(sd,8B)/0x15ED00: cdCallbackPending/Fn/Data/Arg (all zeroed).
- 0x15ECA8/0x15ECAC: batchFreeBytes[0/1] (0x15ECAC == freeBytes[1], NOT a new sym).
- 0x137B00: cdStreamInfo {state@0, error@0x10}; cmdBuf1[0]=cmdBuf2[0]=0.
- Error strings 0x153C50 (SifBindRpcErrorString) / 0x153C78 (989SndSourceFile), out of GP window.
- Callees: initializeSoundSystem(0x11AB20, void, already named), func_0011AFF8
  (0x11AFF8 bind, returns int), func_00116078 (0x116078 error, effectively
  (msg,file,line) -> needs variadic `void f(void*, ...)`), snd_SendIOPCommandAndWait.
- EIDs 0x123456 (rpc) / 0x123457 (cd); error line numbers 0x73 / 0x88.

## Structure (C that is logically correct)
```
6 buffer pointer stores (cmdBufs, retBufs, strBufs, [0] then [1])
timeout=10000; initializeSoundSystem(0); minus=-1;
do { ret=bind(&rpcServer,0x123456,0);
     if(ret<0){ ret=timeout-1; err(errStr,srcFile,0x73); for(;;){} }
     data=(int*)ret; while(ret!=minus) ret--; data=(int*)ret;
  } while(rpcServer.bound==0);
zero cdCallbackPending/Fn/Data/Arg;
do { ret=bind(&cdRpcServer,0x123457,0);
     if(ret<0){ ret=timeout-1; err(errStr,srcFile,0x88); for(;;){} }
     data=(int*)ret; while(ret!=minus) ret--; data=(int*)ret;
  } while(cdRpcServer.bound==0);
data=(int*)&cdStreamInfo; freeBytes[1]=0xFFC; cmdBuf1[0]=0; cdStreamInfo.state=0;
cmdBuf2[0]=0; cdStreamInfo.error=0; snd_SendIOPCommandAndWait(0,4,&data);
freeBytes[0]=0xFFC;   // in call delay slot
```

## Spin loop (both instances identical)
```
beq  v0, s1, exit        ; s1 = minus (-1)
sw   v0, 0(sp)           ; (beq delay) store A
li   v1, -1              ; fresh -1
v0--
nop
.Ltop: nop nop nop nop nop
bnel v0, v1, .Ltop
v0--                     ; (bnel delay)
sw   v0, 0(sp)           ; store B
exit:
```
Countdown starts at 9999: the `bgez v0,spin` DELAY slot is `addiu v0,s2,-1`
(s2=10000), which always runs, so the spin is a fixed ~5000-iter busy wait,
independent of the bind return. The 5-nop `.Ltop` body is alignment padding at
the loop top (0x12DB30 / 0x12DBE8, 8-aligned) — it only materializes when built
in-place at the real address; a standalone probe at another address emits 0 nops.

## CRITICAL: the function must be the FIRST 989snd definition (line 7)

2026-09-18 finding (re-confirmed the block; new gotcha). 989snd functions are
placed SEQUENTIALLY in `.core_text` (no fixed per-function addresses in
SCUS_971.99.ld — the linker script only fixes section boundaries and places
`989snd.o(.text)` in order). So a C function's address is determined by its
position in the .o, which is its position in the SOURCE. In the baseline the
`INCLUDE_ASM(..., snd_StartSoundSystem)` is at **line 7** (the first 989snd
definition), so it links at 0x12DA28. If you write the C body at a later line
(e.g. after `snd_cdStreamInfo` ~line 290), it is compiled AFTER the other
989snd functions and lands ~0x1188 bytes too late, with a huge whole-TU diff
even though the prologue is byte-identical.

Fix for any future attempt: put the C body at line 7 (replacing the INCLUDE_ASM)
and hoist every declaration it uses (the `sndRpcServerT` + `SndCdStreamInfo`
structs, the 6 buffer arrays, the batch/cd-callback globals, the callees, and
the 5-nop macro) to the top of the file above it. This makes the prologue and
address match; it does NOT fix the body (see below). Do this FIRST before any
body iteration, or you will chase a phantom whole-TU shift.

## 9999 folding detail
The original materializes 10000 into a saved register (s2) in the prologue and
forms the spin start as `addiu v0, s2, -1` in the `bgez` DELAY slot (runtime
subtraction). A plain C `data = timeout - 1;` (timeout a local 10000) is
constant-folded by EGC to a literal `li s0, 9999`, which both changes the
register and breaks the delay-slot schedule. Keeping 10000 live in a register
across the bind calls without folding was not achieved.

## expert (GPT-6 Astra) consultation, 2026-09-18
One-shot `expert` consulted. Best concrete recommendation: (1) move
`snd_batchFreeBytes[0]=0xFFC` BEFORE the final `snd_SendIOPCommandAndWait` call
(original puts it in the call delay slot), and (2) try a post-decrement loop
`data = 10000; while (data--) { nops; }`. Testing showed (1) SHRANK the function
to 584 bytes (wrong direction — the original tail is longer than the reordered
C), and (2) compares against 0, not -1, so it cannot produce the original's
`bnel v0, v1` (v1=-1) back-branch. The `data=timeout; ...data--;` split form
OVERFLOWS `.core_text` past the `.core_data` boundary (>600 bytes). So no C form
tested reaches 600 bytes with a matching body; the prior 2026-09-16 attempt got
600 bytes but still had 95 word diffs (prologue alloc + bnel spin).

## Why it does not match (the blocker)
The instruction SET is identical, but EGC schedules the prologue and allocates
registers differently:
- Original keeps **cmdBuf1 in s8** (long-lived, used at the end `sw zero,lo(s8)`),
  and **reloads cmdBuf2** at the end (`lui a0,hi(cmdBuf2)`). My candidate keeps
  cmdBuf1 in s7 and cmdBuf2 in s8 (both saved), so the prologue `move`/`sq`
  interleaving and the whole 0xB0-frame prologue are reordered word-for-word.
- This keep-one-buffer-in-a-saved-reg vs reload-the-other asymmetry, plus the
  exact s1-s8 assignment, is an EGC allocation decision I could not force by
  reordering the C statements or by declaration changes (arrays, section attrs).
- Net effect: 0x228 (552) bytes vs 0x258 (600); a 48-byte cascade from 0x11EA28.

## Things to try next (not yet conclusive)
- The buffer keep-vs-reload split may depend on the exact order the 6 buffer
  addresses are first materialized and on which are dereferenced at the end.
  Original end-deref order: cmdBuf1(s8), cdStreamInfo, cmdBuf2(reloaded).
- Consider whether `data` should be the loop variable (vs a separate `ret`).
- This is the "EGC register-allocation/scheduling mismatch on a large
  multi-call function" class, not a symbol/data problem — all symbols exist.

## last-resort-decompiler (GPT-5.6 Sol) consultation, 2026-09-18
Invoked on this exact target with the full dossier (corrected ground-truth
disasm at 0x12DA28, the in-place C candidate, the per-word diff, and the
specific questions). Its main concrete recommendation was a STRUCTURAL fix to
the countdown: the `bgez` DELAY slot `addiu v0,s2,-1` overwrites the bind
return in BOTH paths, so the spin always starts at `timeout-1` (9999),
independent of the bind result. The C must therefore assign `data = timeout - 1`
UNCONDITIONALLY after the error branch (not inside it, where it is
dead-code-eliminated), use `data` as the spin variable, and scope `timeout`/
`minus` per bind-phase block (the original materializes `li 10000` and `li -1`
once per phase into different registers). Applied all of this.

Result after applying: candidate is 580 bytes (was 544) but still diverges from
word 9 (0x12DA4C) — the PROLOGUE register allocation and scheduling differ:
- Original: `lui s0,0x16` (page), `minus`→s1 in the `jal` delay slot, `sq s0`
  early (word 9). Candidate: `lui s1,0x16` (page), `minus`→s4 HOISTED into the
  prologue (word 25), `sq s0` late (word 28); the sq-save order and the whole
  s0-s8 assignment are shifted.
- Original does NOT constant-fold the countdown (keeps 10000 in s2, forms
  `addiu v0,s2,-1` at runtime). Candidate folds `data = timeout - 1` to a
  literal `addiu s0,0,9999` even with declare-then-assign (`int timeout;
  timeout = 10000;`) — EGC propagates the constant either way.
- Original stores buffer slots [0] GPREL (`sw r,off(gp)`) and [1] ABSOLUTE
  (`lui at,0x16; sw r,lo(at)`); candidate emits all-GPREL (no `lui at`), 3
  words short. The model suggested unsized-array aliases for the [1] slots to
  force ABS, but that does not address the prologue divergence.

The model's remaining matrix (int `snd_SendIOPCommandAndWait` prototype, ABS
aliases, `-mno-split-addresses`) does not touch the prologue allocation, which
is the dominant blocker. No C-level lever (statement order, block/function
scoping, declaration form, section attributes, declare-then-assign) reproduces
EGC's prologue s-register assignment, sq-save order, `minus` delay-slot
placement, or non-folding of `timeout-1`.

## Verdict (2026-09-18, re-confirmed)
BLOCKED. EGC 2.95.2 -G8 -O2 register allocation + scheduling + constant
folding on this 600-byte multi-call function do not match the original and are
not controllable from C source. Reverted to INCLUDE_ASM (line 7); full boot ELF
parity intact. See blocked.json.
