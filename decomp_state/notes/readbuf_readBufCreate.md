# readBufCreate__FP7ReadBuf (code/game/movie/readbuf.cpp)

- Original: 24 bytes (`0x18`) at file offset `0x13C8C0` in `assets/boot_elf.elf`,
  vram `0x0023B940`. Object-relative `.text+0x0` in `readbuf.o`.
- Semantics (Ghidra `FUN_0023b940` agrees): ring-buffer init —
  `capacity(self+0x50008) = 0x50000`, `count(self+0x50004) = 0`,
  `putPos(self+0x50000) = 0`.
- Original instruction sequence:
  ```
  lui   $v0, 0x5        # v0 = 0x50000
  addu  $a0, $a0, $v0   # a0 = self + 0x50000 (points at putPos)
  sw    $v0, 8($a0)     # capacity = 0x50000
  sw    $zero, 4($a0)   # count    = 0
  jr    $ra
      sw $zero, 0($a0)  # putPos   = 0   (delay slot)
  ```

## The matching problem (EGC 2.95.2 -O2 store scheduling)

Three independent stores, one delay slot: the compiler decides which store
goes into the `jr $ra` delay slot. Empirically (standalone `-S` experiments,
all 6 source-order permutations of the three assignments on a plain
non-volatile struct):

- EGC always emits `li $v0,0x50000; addu $a0,$a0,$v0` first.
- It puts ONE of the two zero-stores before the `jr` and the other into the
  delay slot, independent of source order in several cases:
  - source order (cap, putPos, count) -> cap, cnt, jr + putPos-slot  [MATCH]
  - source order (count, putPos, cap) -> cap, cnt, jr + putPos-slot  [MATCH]
  - source order (putPos, cap, count) -> cap, cnt, jr + putPos-slot  [MATCH]
  - the other three orders           -> cap, putPos, jr + cnt-slot   [no match]

The chosen candidate is the first-listed ordering.

## Struct change

`ReadBuf` fields in readbuf.cpp were changed from `volatile u32` to plain
`u32`. The volatile qualifiers had been guessed by an earlier pass "for
consistency with VoBuf"; they are not needed here and plain fields are what
the scheduler experiments used. Safe: the only other matched function in this
file (`readBufDelete`) does not touch any field, so no currently-matching code
changes behavior.

## C candidate

```cpp
typedef struct ReadBuf {
    u8 data[0x50000];
    u32 putPos;
    u32 count;
    u32 capacity;
} ReadBuf;

void readBufCreate(ReadBuf* self) {
    self->capacity = 0x50000;
    self->putPos = 0;
    self->count = 0;
}
```

Mangling: free function `void readBufCreate(ReadBuf*)` mangles under old-GCC
C++ to exactly `readBufCreate__FP7ReadBuf` (same rule as readBufDelete, see
notes/readbuf_readBufDelete.md). No extern "C".

## Verification (mechanical)

- `make build/code/game/movie/readbuf.o`: compiles clean under EGC 2.95.2 `-O2`.
- `objdump -d build/code/game/movie/readbuf.o`: symbol
  `readBufCreate__FP7ReadBuf` at `.text+0x0`, size `0x18`, words
  `3c020005 00822021 ac820008 ac800004 03e00008 ac800000` — byte-identical to
  the original slice at file offset `0x13C8C0`.
- `nm -n`: `readBufDelete__FP7ReadBuf` still at `.text+0x18`, nonmatching
  fragments after that unchanged.
- Full build: `make` then `cmp build/boot_elf.elf assets/boot_elf.elf` passes
  (ELF_MATCH).

## Remaining readbuf.cpp targets (same family)

- `readBufBeginPut__FP7ReadBufPPUc` (`0x23B960`): free = capacity - count;
  out = self + putPos; returns free. Watch which register holds `out` vs the
  return value (both are words of `self+...`).
- `readBufEndPut__FP7ReadBufi` (`0x23B990`): traps if capacity==0;
  count += min(n, free); putPos = (putPos + n) % capacity.
- `readBufBeginGet__FP7ReadBufPPUc` (`0x23B9D8`): out = self +
  ((putPos - count) + cap) % cap; returns count.
- `readBufEndGet__FP7ReadBufi` (`0x23BA20`): count -= min(n, count).
