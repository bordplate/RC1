# snd_StreamSafeCdCallback (code/989snd/ee/989snd.c) — MATCHED 2026-09-11

`int snd_StreamSafeCdCallback(int callback)` at vram `0x0012EF28` (file offset
`0x2EF48`), 0x30 (48) bytes. Registers the CD status callback for the
stream-safe CD session: when no stream session is active the callback goes to
the SCE disc-driver path (`func_00120678`); otherwise the previous callback
stored in `snd_cdStatusCallback` (0x15EC90) is replaced and returned.

## Identity / context

- Sole caller: `music_registerCdCallback` in `code/game/music.cpp`
  (`snd_StreamSafeCdCallback(stream_updateCdStatus)`; music.cpp keeps its
  historical `extern "C" void (void(*)(int))` declaration — C linkage does not
  unify types across TUs, and the call site passes the pointer in a0 either
  way, so the matched caller bytes are unaffected).
- Slot consumer: the IOP return handler at 0x12DE0C does `lw v0, -0x7F70(gp)`
  (= 0x15EC90) then `jr v0` with `a0 = 1` — a 32-bit text address, invoked on
  CD status updates. The slot is written with a full-register `sq` (64-bit
  store of a zero-extended int) and read back with a 32-bit `lw`.
- Sibling family: `snd_StreamSafeCdGetError` (0x12EF08, matched) shares the
  `snd_cdStreamActive` (0x15EC8C) gate; `snd_cdStatusCallback` sits in the
  same .core_lit window (0x15EC90, added to config/symbols.txt).

## Replacement

```c
int snd_StreamSafeCdCallback(int callback) {
    int old;

    if (!snd_cdStreamActive)
        return func_00120678(callback);

    old = snd_cdStatusCallback;
    snd_cdStatusCallback = callback;
    return old;
}
```

`func_00120678` is a generated-SCE-library routine (address-based name
retained in this C TU, prototype comment per file convention). `int` models
both the parameter and the slot (project ABI: int and pointers are 32 bits);
a function-pointer parameter/extern shape compiles to the same 48 bytes but
with pointer/int conversion warnings, so the warning-free `int` form is used.

## Codegen facts that matter

- The matched shape is an EARLY-RETURN condition plus a block-leading local:
  `if (!A) return g(a);` makes EGC put the FALSE path (`jal func_00120678`)
  in the taken branch and the TRUE continuation on fall-through, matching the
  original `beq v0,zero,L0; sq ra,delay`. Plain `if (A) {...} else return
  g(a);` (and braced variants) emit `bne` with the true path taken — wrong
  direction.
- Loading the old value into `old` BEFORE the store is what reproduces the
  original true-path order `lw v0,B; b L1; <sq a0,B in the b delay slot>`.
  Store-first (`B = a; return B;`) emits `sw` (or `sq`) before the load and
  two epilogue `lq ra` loads; `return snd_cdStatusCallback;` after the store
  gets copy-propagated to `move v0,a0` (load dropped).
- The single shared epilogue (`lq ra` once at the exit block, entered by
  fall-through from the false path and by the true path's `b L1`) falls out
  of that same store-first-in-program-order, store-second-in-schedule shape;
  every variant that exits the true path without a trailing store before the
  skip branch duplicated the `lq ra`.
- Verified via `tools/decomp_probe.py` (candidates a–j in
  /tmp/opencode/tail/cdc_*.c): only the early-return + old-local form matches
  48/48 with zero differences.

## Adjacent dead tail (func_0012EF58) — separate queue entry, left todo

```
0x12EF58: addiu sp, sp, 0x20     <- unreachable (file 0x2FED8)
0x12EF5C: nop
0x12EF60: addiu sp, sp, 0x20
0x12EF64: nop
```

12-byte fragment (2 x `addiu sp,sp,0x20` + nop) at the 8-aligned address right
after this function's `jr ra; <addiu sp,sp,0x10>` epilogue — the classic
dead-tail artifact family (see 989snd_snd_StopAllStreams.md and the systemic
~48-fragment analysis in AGENTS.md/help_msg_string__Fi.md). Ghidra has no
function at 0x12EF58 and no xrefs to it. Its 0x20 frame value does not even
match this parent's 0x10 frame. It remains its own queue target; no C form
has been shown to regenerate it with local EGC 2.95.2, and a blocker entry
requires the last-resort-decompiler escalation first. INCLUDE_ASM retained.

## Verification

- Probe: candidate object byte-compared at the original address, 48/48 equal,
  no differences.
- `make split && make -B -j2 && cmp build/boot_elf.elf assets/boot_elf.elf`
  → byte-for-byte; split moved the generated assembly to
  `code/_generated/matchings/989snd/ee/989snd/snd_StreamSafeCdCallback.s`.
- `tools/decomp_status.py --count`: 723 → 722.
