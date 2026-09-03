# readBufDelete__FP7ReadBuf (code/game/movie/readbuf.cpp)

- Original: 8 bytes (`0x8`) at file offset `0x13C8D8` in `assets/boot_elf.elf`,
  vram `0x0023B958`. Object-relative `.text+0x18` in `readbuf.o`.
- Semantics: empty function taking one `ReadBuf*` — the buffer is a
  statically-sized struct (data + trailing metadata), so "delete" does nothing.
- Original instruction sequence:
  ```
  jr     $ra
     nop
  ```
- Ghidra decompile of `FUN_0023b958`: `return;`.
- Sole xref: UNCONDITIONAL_CALL from vram `0x0023AA88` (inside the movie read
  loop function `FUN_0023aa68`, see `readMpeg__FP8VideoDecP7ReadBufP7StrFile`
  family), so the symbol is referenced and must be emitted.

## ReadBuf layout (derived from sibling decompiles)

`readBufCreate` (vram `0x23B940`) writes `*(self+0x50008)=0x50000`,
`*(self+0x50004)=0`, `*(self+0x50000)=0`. Siblings confirm a ring buffer:

- beginPut (`0x23B960`): free = capacity(0x50008) - count(0x50004);
  out = self + putPos(0x50000); returns free.
- endPut (`0x23B990`): traps if capacity==0; count += min(n, free);
  putPos = (putPos + n) % capacity.
- beginGet (`0x23B9D8`): out = self + ((putPos - count) + cap) % cap; returns count.
- endGet (`0x23BA20`): count -= min(n, count).

So: `u8 data[0x50000]` followed by trailing words putPos (0x50000),
count (0x50004), capacity (0x50008). Fields declared volatile in the local
struct for consistency with the VoBuf treatment, which will help the
begin/end decompiles match.

## C candidate

```cpp
void readBufDelete(ReadBuf* self) {}
```

Old-GCC C++ mangling (see notes/vobuf_voBufDelete.md): free function
`void readBufDelete(ReadBuf*)` mangles to exactly `readBufDelete__FP7ReadBuf`.
No extern "C", no manual name. Zero operations to schedule; EGC `-O2` emits
the canonical `jr $ra; nop`.

## Verification (mechanical)

- `make build/code/game/movie/readbuf.o`: compiles clean under EGC 2.95.2 `-O2`.
- `objdump -d build/code/game/movie/readbuf.o`: symbol
  `readBufDelete__FP7ReadBuf` at `.text+0x18`, size `0x8`, words
  `03e00008 00000000` — byte-identical to original slice (`0800 e003 0000 0000`)
  at file offset `0x13C8D8`.
- Zero relocations in the object (nothing in `.text` range `0x18..0x20`).
- Full build: `make` then `cmp build/boot_elf.elf assets/boot_elf.elf` passes.

## Remaining readbuf.cpp targets (same family)

- `readBufCreate__FP7ReadBuf` — 3 trailing-word stores, likely needs the same
  volatile-field scheduling care as voBufReset (independent stores into the
  jr delay slot).
- `readBufBeginPut/EndPut/BeginGet/EndGet` — ring-buffer math above; beginGet
  has a `% cap` modulo that EGC may lower to a branchless or branched sequence;
  endPut traps (`break 7`) on capacity==0.
