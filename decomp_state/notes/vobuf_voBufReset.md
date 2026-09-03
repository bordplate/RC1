# voBufReset__FP5VoBuf (code/game/movie/vobuf.cpp)

- Original: 16 bytes at file offset `0x13E168` in `assets/boot_elf.elf`.
  Splat vram address `0x0023D1E8`; object-relative `.text+0x58`
  (vobuf subsegment starts at file `0x13E110`).
- Semantics: ring-buffer reset — zeroes `count` (offset `0xC`) then `head`
  (offset `0x8`) of a `VoBuf`.
- Original instruction sequence (file-byte order):
  ```
  sw   $zero, 0xC($a0)   # count = 0
  sw   $zero, 0x8($a0)   # head  = 0
  jr   $ra
  nop
  ```

## The matching problem (EGC 2.95.2 -O2 scheduling)

The obvious C (`self->count = 0; self->head = 0;` on a plain struct) does NOT
match. EGC's post-register-allocation / delay-slot scheduler reorders the two
independent zero-stores and hoists the second one into the `jr $ra` delay
slot:

```
sw   $zero, 0x8($a0)
jr   $ra
   sw $zero, 0xC($a0)    <- scheduled into delay slot
nop
```

The original kept both stores in program order before the `jr`, with a real
`nop` in the delay slot. This happens when the compiler treats the two stores
as *volatile* (must keep source order, not schedulable across the branch).

## Fix

Declare the stored fields `volatile` in the struct:

```cpp
typedef struct VoBuf {
    void* data;             // 0x0
    void* tags;             // 0x4
    volatile u32 head;      // 0x8
    volatile u32 count;     // 0xC
    u32 capacity;           // 0x10
} VoBuf;

extern "C" void voBufReset__FP5VoBuf(VoBuf* self) {
    self->count = 0;
    self->head = 0;
}
```

Both `head` and `count` need to be volatile; making only one volatile still
lets EGC reorder. Confirmed by a standalone `-S` experiment (f3 variant).

## Verification (mechanical)

- `make` builds; `cmp build/boot_elf.elf assets/boot_elf.elf` passes
  (ELF_MATCH).
- `objdump -d -j .text build/code/game/movie/vobuf.o`: symbol
  `voBufReset__FP5VoBuf` at `.text+0x58`, size `0x10`, words
  `ac80000c ac800008 03e00008 00000000` — identical to the original slice.

## VoBuf layout (inferred from sibling nonmatching asms)

Siblings in `code/_generated/nonmatchings/game/movie/vobuf/` reference
`this+0x0` (data ptr), `this+0x4` (tag ptr), `this+0x8` (head index),
`this+0xC` (count), `this+0x10` (capacity). `voBufIsFull` = `count==capacity`,
`voBufIsEmpty` = `count<1`, `voBufDecCount` guards `count>0`.

## Next candidates in this file (same pattern)

- `voBufIsEmpty` (`count<1`) — plain, 8+ bytes.
- `voBufDecCount__FP5VoBuf` — branch + dec, no delay-slot trap expected.
- `voBufIsFull__FP5VoBuf` — xor + sltiu.
