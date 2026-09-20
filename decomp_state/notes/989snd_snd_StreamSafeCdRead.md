# snd_StreamSafeCdRead (0x12ED58, 176 bytes) — MATCHED

Issues a raw CD streaming read while a stream-safe session is active. In
`code/989snd/ee/989snd_post.c` (GNU-compat TU, `-G8 -O2 -ffast-math
-fno-exceptions`).

```c
int snd_StreamSafeCdRead(int arg0, int arg1, int arg2) {
    int buf[3];

    if (!snd_cdStreamActive)
        return func_00121450(arg0, arg1, arg2);
    if (snd_StreamSafeCdSync(SND_CD_SYNC_MODE_CHECK) == 1)
        return 0;
    snd_cdStreamInfo.cd_busy = 1;
    snd_cdStreamInfo.cd_error = 0;
    buf[0] = arg0;
    buf[1] = arg1;
    buf[2] = arg2;
    snd_SendIOPCommandNoWait(SND_IOP_CMD_CD_STREAM_READ, 0xC, (char*)buf, 0, 0);
    snd_cdSyncPending = 1;
    snd_cdStreamEndPending = 0;
    return 1;
}
```

## 2026-09-20 refactor: named IOP command constants

The protocol-level naming pass this function was held for landed in
`code/include/989snd_iop.h`: a 33-entry `SND_IOP_CMD_*` enum covering every
IOP command issued in C across the 989snd TUs (31 via snd_SendIOPCommand*
here, plus SND_IOP_CMD_EXECUTE_BATCH 0x4D in snd_SendCurrentBatch and
SND_IOP_CMD_CD_BANK_LOAD 0x57 in snd_BankLoadFromEE_CB), and the two
`SND_CD_SYNC_MODE_*` values for snd_StreamSafeCdSync. Opcode meanings are
evidenced by the verified EE wrapper that issues each command; the same
opcode values appear in the same-named wrappers of the Deadlocked PAL
989snd library (reference/dl/989snd/ee/989snd.c). The refactor applied the
constants to all 34 C call sites in 989snd_post.c, 989snd_bankload.c, and
989snd_flush.c (the only TUs with C call sites). Data-size args and the
boolean 0/1 states stay raw (STYLEGUIDE buffer-size/boolean exception).
The same pass also renamed the placeholder wrapper parameters
(`a`/`b`/`argN` -> the Deadlocked debug-symbol names: `lbn`/`sectors`/`buf`
here, plus `bank`/`which`/`vol`/`mode`/`groups`/`handle`/`stream`/`core`/...
across the 989snd_post.c wrappers, and `mode` in snd_StreamSafeCdSync, whose
internal busy test now reads `mode == SND_CD_SYNC_MODE_CHECK`). Parameter
and local names are codegen-neutral in C; the local staging array in
snd_StreamSafeCdRead became `data` to match the DL source now that the
buffer parameter is `buf`. Clean `make clean && make split && make -j2`
+ `cmp` byte-for-byte, and `tu_assembler_diff` reports 54/54 (post), 1/1
(bankload), 1/1 (flush) function-level matches. The refactor.json entry
`989snd_post_snd_StreamSafeCdRead` was cleared.

## Semantics

Three int args. If no stream-safe session is active (`snd_cdStreamActive == 0`),
delegates to the raw SCE CD read `func_00121450(arg0, arg1, arg2)` and returns
its result. Otherwise it syncs the CD
(`snd_StreamSafeCdSync(SND_CD_SYNC_MODE_CHECK)`); a return of 1 means
"already handled / stream ended" and the function returns 0. On the live
path it marks the stream busy (`cd_busy = 1`), clears the error
(`cd_error = 0`), stages the three args on the stack, queues
SND_IOP_CMD_CD_STREAM_READ (0x38) with 0xC (12) bytes of data, then flags
`snd_cdSyncPending = 1` and `snd_cdStreamEndPending = 0`, and returns 1.
The return value is `(snd_StreamSafeCdSync(SND_CD_SYNC_MODE_CHECK) != 1)`.

## Globals / callees

- `snd_cdStreamActive` (0x15EC8C, GPREL load in the prologue).
- `snd_cdStreamInfo` (0x137B00, `volatile SndCdStreamInfo`, absolute
  self-based `lui`/`sw`; `cd_busy` at +0, `cd_error` at +0x10).
- `snd_cdSyncPending` (0x15EC94, GPREL store), `snd_cdStreamEndPending`
  (0x15EC98, GPREL store).
- `func_00121450` — the raw CD streaming read in the generated SCE library. It
  is an `alabel` at +4 inside `func_0012144C` in `code/_generated/sce/lib.s`
  (the `pref` at 0x12144C is a prefetch hint; the real prologue starts at
  0x121450). Declared here as `int func_00121450(int, int, int)`; exact SDK API
  identity unresolved, so the address-based name is retained in this C TU (same
  convention as the neighbouring `func_00121630`/`func_00120678`/
  `func_001216C8`/`func_00120C30`).

## Codegen facts

The natural C matched byte-for-byte on the first build; no scheduling
reordering was needed. Two points worth recording:

1. **The early-return call reuses the incoming argument registers.** The
   prologue spills the three args to `s0`/`s2`/`s3` (needed on the read path to
   fill `buf[0..2]`), but leaves `a0`/`a1`/`a2` holding the original values.
   EGC then passes the args to `func_00121450` straight through `a0`/`a1`/`a2`
   (`jal 0x121450` with no `move` setup), so `return func_00121450(arg0, arg1,
   arg2);` reproduces it exactly. Declaring the callee with exactly three int
   params matters: the original does not set up a fourth arg (`a3` is left as
   the caller's leftover).
2. **The `volatile` `SndCdStreamInfo`** (shared with the StreamSafe family)
   keeps the out-of-window 0x137B00 base from being CSE'd/folded; `cd_busy`
   (store of the `s1 == 1` constant) and `cd_error` (store of zero) are emitted
   as two separate self-based `lui`/`sw` sequences in the original's order.

Verified: `tu_assembler_diff` on `989snd_post.o` reports the function
176/176 bytes identical (all 54 funcs in the TU match); full `make` +
`cmp build/boot_elf.elf assets/boot_elf.elf` byte-for-byte.
