# func_0022DDD8 (0x22DDD8, 44 bytes)

Matched 2026-09-07 with default flags, first probe attempt (after the 3-store
reorder was understood).

```c
extern "C" void func_0022DDD8(int a, long b) {
    int c = (int)b;
    if (c == 0) return;
    *(int*)c = a;
    if (a != 0) return;
    *(int*)(c + 0x18) = 0;
    *(int*)(c + 0x1C) = 0;
    *(char*)(c + 4) = 0;
}
```

Purpose: a sound-slot callback registered with the 989snd stream system by
`sound_update` (0x22CA50) via `FUN_0012e4c0`/`FUN_0012e448` (snd_ bank/stream
callers pass this address as a fn-ptr). It stores `a` into the slot's first
int; when `a == 0` it additionally clears the slot's flag byte (+0x4) and two
trailing ints (+0x18, +0x1C). It is the "set-and-conditional-clear" sibling of
the already-matched `func_0022DD78` (0x22DD78, plain `if (c) *(int*)c = a;`).
Kept the `func_` name for consistency with that sibling (exact slot struct
semantics not worth guessing a name for).

Notes:
- Argument convention matches the matched sibling: `long b` (64-bit pointer
  arriving in $a1), truncated to a signed `int c = (int)b`. Using the signed
  `int` as a pointer makes EGC emit the sign-extend pair `dsll32 a1; dsra32
  a1` (NOT the zero-extend `dsll32; dsrl32`) that the original has — this is
  the crux. A `u32`/unsigned pointer would zero-extend and miss by one word.
  `char` is used for the +0x4 store so no `types.h` include is needed
  (sound.cpp only pulls in common.h); `char` and `u8` both emit `sb zero,
  4(a1)`.
- The tail is three independent constant stores to a REGISTER base ($a1) —
  `sb 0,4(a1); sw 0,0x18(a1); sw 0,0x1C(a1)` — all before `jr ra` with a `nop`
  delay slot (none lands in the delay slot, because they sit after the
  `bnez a0` branch, unlike the plain 3-store epilogue case). EGC emits these
  in the fixed `stmt3; stmt1; stmt2` permutation (AGENTS.md register-base
  observation). The original order is `4, 0x18, 0x1C`, so the source must be
  written `0x18, 0x1C, 4` (stmt1=0x18, stmt2=0x1C, stmt3=4) to reproduce it.
  Writing them in the "natural" offset order `4, 0x18, 0x1C` makes EGC emit
  `0x1C, 4, 0x18` (wrong).
- The `*a1 = a` store lands in the `bnez a0` branch delay slot automatically;
  the `if (c == 0) return;` / `if (a != 0) return;` guards map to `beqz a1`
  (nop delay) and `bnez a0` respectively. Both early-returns share the single
  `jr ra` exit.
- No prologue (leaf, no calls, no stack).
- Probe: decomp_state/probes/sound_func_0022DDD8_match.cpp, all 44 bytes match
  (decomp_probe.py `match: true`, 0 differences).
- Full clean build (`make clean && make split && make -j2`) + `cmp
  build/boot_elf.elf assets/boot_elf.elf` byte-for-byte. Nonmatching count
  747 -> 746.
