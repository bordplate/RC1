# snd_BankLoadFromEE_CB (0x12E088, 272 bytes) — MATCHED

CD-bank-load RPC callback. `snd_BankLoadFromEE_CB(void* ee_loc, SndCompleteProc
cb, u64 user_data)` arms the CD load callback and issues the 0x57 bank-load RPC:

```
snd_cdLoadError = 0;
if (snd_cdCallbackPending != 0)            { report(BankLoadFromEEProgressString); return; }
if (snd_StreamSafeCdSync(1) == 1)          { report(BankLoadFromEECdBusyString); return; }
snd_cdBankLoadRequest = (int)ee_loc;
snd_cdCallbackArg    = 0xFFFFFFFF;          // "no result yet" sentinel
snd_cdCallbackFn     = cb;
snd_cdCallbackData   = user_data;
while (sceSifCheckStatRpc(&snd_cdRpcServer) != 0) {
    report(NonIdleErrorString); snd_FlushSoundCommands(); FlushCache(0);
}
snd_cdCallbackPending = 1;
sceSifCallRpc(&snd_cdRpcServer, 0x57, 1, &snd_cdBankLoadRequest, 4,
              &snd_cdCallbackArg, 4, 0, 0);
```

## The problem: one function needs THREE access forms

The original mixes, in a single function:
- **Absolute self-based tight pairs** for in-window arg/data
  (`lui at; sw/sd` for `snd_cdCallbackArg` 0x15ED00 and `snd_cdCallbackData`
  0x15ECD8, and the `la` into t1 for the RPC 6th arg).
- **GP-relative** for in-window small `snd_cdCallbackPending` (0x15ECC8) load +
  store, `snd_cdCallbackFn` (0x15ECD0) store, and `snd_cdLoadError` (0x15EC88)
  zero-store (in the prologue `noreorder` region).
- **Hoisted CSE splits** for out-of-window `snd_cdBankLoadRequest` (0x137B40),
  `snd_cdRpcServer` (0x15EBE8), and the three error-string pointers.

EGC is uniform per TU: with splitting OFF every symbolic access is a hoisted
split; with it ON every in-window access is a tight `la`. No single EGC+GNU
configuration reproduces the mix. Constant-address casts (the only form that
does) are forbidden in source (overlay compatibility).

## The solution: SN assembler + selective `.extern` seeding

The bankload object is isolated to its own Splat range and assembled with
**ps2eeas (SN)** instead of GNU. ps2eeas is single-pass and expands a bare
`lw/sw/la r,sym` to a GPREL16 access **only when a `.extern sym,N` declaration
already appeared earlier in the file** (or inside `.set noreorder`); otherwise
it emits the absolute self-based `lui at; <access>` tight pair. EGC emits all
its `.extern` declarations at the END of the file, so a plain in-window load is
absolute under SN — except the ones we seed.

Declarations (in `989snd_bankload.c`):
- `snd_cdCallbackArg`, `snd_cdCallbackData`, `snd_cdCallbackFn`,
  `snd_cdCallbackPending`, `snd_cdLoadError` = plain small scalars (NO `.data`);
  `snd_cdBankLoadRequest`, `snd_cdRpcServer`, and the three error strings = `.data`
  (out-of-window, explicit hoisted splits).
- File-scope seed before the function:
  `asm(".extern snd_cdCallbackPending, 4\n.extern snd_cdCallbackFn, 4");`

Effect under SN: pending load + fn store + pending store expand **GPREL**
(seeded); arg store + data store + RPC-arg `la` stay **absolute tight pairs**
(unseeded); loadError store is GPREL (it sits in the prologue `noreorder`
region, which forces GPREL without seeding); req/server/strings are hoisted
splits. That is exactly the original's per-symbol mix → **272/272 words, 0 diffs**.

This is the documented project `.extern`-seeding technique (menu.cpp
`menu_isSelectionCountZero__Fv`), applied per-object via a Splat range split so
the SN assembler stays local.

## Integration (Splat range split, 3 objects)

`snd_BankLoadFromEE_CB` sits mid-range (file 0x2F008–0x2F118), so one source
file cannot give it its own object while the siblings keep GNU (two subsegments
sharing a source would place that object twice → link overlap; a whole-TU-SN
build grows `.core_data` into `.core_text`). `config/RC1.yaml` now splits the
989snd segment into three:

```
[0x2e9a8, c, 989snd/ee/989snd_pre]      (GNU)
[0x2F008, c, 989snd/ee/989snd_bankload] (SN, default -snas)
[0x2F118, c, 989snd/ee/989snd_post]     (GNU)
```

`989snd_pre.c` / `989snd_post.c` are byte-splits of the old `989snd.c`
(INCLUDE_ASM paths retargeted to `_pre`/`_post`); the Makefile GNU list covers
`989snd_pre.o` and `989snd_post.o` but not `989snd_bankload.o`.

## Cross-reference

The sibling `snd_BankLoadByLoc` (0x12DF20, in `989snd_pre.c`) is BLOCKED on the
same mixed-access-form problem (see 989snd_snd_BankLoadByLoc.md); that note's
"UNSOLVABLE" conclusion predates this SN + `.extern`-seeding route. It may now
be unblockable by isolating it into its own SN object the same way. Not done
here (separate target).

## Symbols
Named (kept, real globals): `snd_cdBankLoadRequest` (0x137B40),
`snd_cdCallbackArg` (0x15ED00), `snd_cdCallbackFn` (0x15ECD0),
`snd_cdCallbackData` (0x15ECD8), `snd_cdRpcServer` (0x15EBE8), `snd_cdLoadError`
(0x15EC88), `snd_cdCallbackPending` (0x15ECC8). New:
`snd_BankLoadFromEEProgressString` (0x153EC0), `snd_BankLoadFromEECdBusyString`
(0x153EF8). The shared error string stays the committed name
`snd_NonIdleErrorString` (0x153D20, "989snd.c: RPC still nonidle!").

## Verification
Standalone probe (EGC + ps2eeas, the exact source): 272/272 words, 0 diffs.
Full clean build (`make clean && make split && make -j2`) +
`cmp build/boot_elf.elf assets/boot_elf.elf` passes. Splat moved the function to
`code/_generated/matchings/989snd/ee/989snd_bankload/`.
