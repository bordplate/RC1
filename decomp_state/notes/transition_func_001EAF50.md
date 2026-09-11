# func_001EAF50 -> Help_LoadMsgs (matched 2026-09-11)

## Semantics
Loads a help message set for the transition:

- `t = *(int*)0x15EF64` — pointer to a per-set offset table (absolute lui/lw in the
  original, so the original used constant-address casts, not plain gp-window symbols).
- `b = *(int*)0x15EF60` — pointer to the help message data base.
- `elem = ((int*)t)[set]`; `node = (char*)b + elem` (raw 32-bit addu, no scale).
- `HelpMsgCount` (0x1996FC, out of gp window) = `*node` (message count for the set).
- `HelpMsgs` (0x15F6A0, in gp window) = `(struct HelpMsg*)(node + 8)` (entry array).

void return; all three call sites ignore v0:
- `Transition_LoadWad` (0x1EA830): loop i=0..7 calling `Help_LoadMsgs(i)`, then one
  `Help_LoadMsgs(0)` afterwards.
- `Transition_DoTransition__Fv` (0x1EBA10).

The loader loop in Transition_LoadWad then rewrites each 16-byte entry's text pointer
relative to `HelpMsgs - 8`, and `Help_FindIndex` (0x1FDCA0) reads the count at
0x1996D0+0x2C = 0x1996FC. The 0x15EF60/64 pointers are only read in the boot ELF
(overlay code sets them).

## Renames
- `func_001EAF50` -> `Help_LoadMsgs__Fi` (config/symbols.txt, transition section).
- `Help` (0x0196FC) -> `HelpMsgCount` (config/symbols.txt, help section); generated
  references (pause_post func_0021D2C8, data.data dlabel) follow on re-split.

## Match details
Original layout (14 insns):
`t pair -> a1 (fused); sll a0; b pair -> v1 (fused); lui a2 (hoisted, 4 slots before
the store); addu; elem lw -> v0; addu v1 in-place; count lw -> a0; addiu v1,8;
sw count main; jr ra; sw msgs(gp) delay`.

No natural C form reproduced this with the project flags. ~95 variants tested
(const-cast loads with int/char*/unsigned*/int*/unsigned/long locals, both operand
orders, local cnt / local n8, volatile, &HelpMsgCount forms, both store orders,
C and C++ frontends) under sched / -fno-schedule-insns / -fno-schedule-insns2 /
both / -mno-split-addresses:

- Default sched, char* raw-add: 8 diffs — the hoisted lui lands in a2 and the count
  in a0 (both correct), but the base loads come out b->v0 / elem->v1 (swapped) and
  the two stores swap (gp main / abs delay).
- Default sched, int* form: 7 diffs — allocation matches (b->v1, elem->v0) but the
  scaled add emits an extra `sll` and `addiu 32`.
- -fno-schedule-insns, char* raw-add: 5 diffs — allocation and store order match,
  but the store HI16 lands in a0 one slot late and the count value in v0.
- -fno-schedule-insns with `int cnt` local: 6 diffs — count->a0 becomes correct,
  HI16 still late and in v0.

Last-resort escalation (last-resort-decompiler, GPT-5.6 Sol) found the route:
pin the store's page half to $a2 via a hard register variable plus a zero-byte
`"+r"` barrier, and pin the count value to $a0. With the indexed-array source form
(`((int*)(...))[set]` — manually forming `t + (set << 2)` regresses to the 2-diff
`$a1` address allocation), `-fno-schedule-insns` then reproduces all 56 bytes:

```cpp
register int helpCountPage asm("$6") = 0x1A0000;
asm volatile("" : "+r"(helpCountPage));
register int count asm("$4") = *(int*)node;
*(int*)(helpCountPage - 0x6904) = count;   // 0x6904 = 0x1A0000 - 0x1996FC
```

Verified variants from the escalation: hard `&HelpMsgCount` in $a2 (2 diffs,
destructive $a1 table address); page barrier without the count pin (2 diffs, count
stays in v0); reusing `set` for the count (2 diffs); binding `&HelpMsgCount` with a
barrier (extra instruction).

## Flag
`game/transition.o` receives `PRIVATE_COMPILE_FLAGS = -fno-schedule-insns`
(Makefile, alongside the menu group). The TU contained no previously matched C
functions, so no existing match is affected.

## Verification
Standalone probe: 56/56 bytes, 0 differences (Help_LoadMsgs__Fi). Clean
`make clean && make split && make -j2` plus `cmp build/boot_elf.elf
assets/boot_elf.elf` passes. Count: 721 -> 720 nonmatching.
