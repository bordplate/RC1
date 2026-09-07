# readBufEndPut__FP7ReadBufi (code/game/movie/readbuf.cpp) — MATCHED 2026-09-07

`void readBufEndPut(ReadBuf* buf, int n)` at vram `0x0023B990` (file offset
`0x13C910`), 0x44 bytes (17 words). Commits `n` bytes just written into the
ReadBuf ring buffer (the counterpart of the matched `readBufBeginPut`, which
reserved the space): clamps `n` to the free space, advances `putPos` with
wrap-around, and increments `count`.

## Semantics

```
m      = min(n, capacity - count)          (signed compare)
putPos = (putPos + m) % capacity           (signed modulo, wraps the write head)
count += m
```

ReadBuf layout (verified against all four sibling accessors):
`data[0x50000]`, then `putPos` (+0x50000), `count` (+0x50004), `capacity`
(+0x50008). Callers (movie/video decode paths) write into the pointer returned
by `readBufBeginPut`, then call this with the number of bytes written.

## Replacement (code/game/movie/readbuf.cpp)

```c
void readBufEndPut(ReadBuf* buf, int n) {
    int space = buf->capacity - buf->count;
    int m = (n < space) ? n : space;
    buf->putPos = (buf->putPos + m) % buf->capacity;
    buf->count += m;
}
```

## Codegen facts that matter

- **Signed fields are required.** The original uses `slt` for the min and
  `div` (signed) + `mfhi` for the wrap-around modulo. With the struct fields
  as `u32` (as they were), EGC emits `sltu`/`divu`. The ReadBuf struct in
  readbuf.cpp therefore now declares `putPos`/`count`/`capacity` as `int`.
  This is codegen-invisible for the already-matched siblings:
  readBufCreate (constant stores), readBufBeginPut (`subu` + pointer add),
  readBufEndGet (`count` was already `int`) — all re-verified byte-identical
  by decomp-verifier after the switch.
- **EGC div-by-zero debug guard idiom.** Before/around every runtime `div`
  EGC emits a degenerate 2-instruction guard: a no-op branch on the divisor
  (`beqzl cap, +8` — target is exactly the fall-through) whose delay slot
  carries `break 0,7` (word `CD010000`). The branch never changes control
  flow; the break is an unconditional (unreachable-in-practice) debug marker.
  The matched `snd_GetDopplerPitchMod` (`arg0*1524/741`) contains the same
  `break 0,7`, confirming the local EGC reproduces it.
- **Statement order flips the allocation.** The `putPos = (putPos+m) % cap;`
  statement must come BEFORE the `count += m;` statement. Only that order
  makes EGC allocate capacity→a3 / count→t0 / putPos→a2 and schedule the
  guard immediately after the three loads. Count-first (or splitting
  `putPos += m` into its own statement, which adds a second putPos store)
  yields the mirror allocation (capacity→t0 / count→a3) and the guard after
  the count store — 11-word diffs. Fuzzed 8 standalone variants
  (`/tmp/opencode/readbuf2/fuzz.py`): putPos-first (`buf->count += m` after),
  explicit-temp form (`int pp = putPos+m; int c = count+m; ...`), and
  spelled-out `buf->count = buf->count + m;` all match 17/17; the others
  don't. The `+=` form with putPos first was chosen (matches sibling style).
- Original word list (all 17 reproduced):
  `lui v0,5; addu a0,a0,v0; lw a3,8(a0); lw t0,4(a0); lw a2,0(a0);
   beqzl a3,+8; break 0,7; subu v0,a3,t0; slt v1,a1,v0; movn v0,a1,v1;
   addu a2,a2,v0; addu t0,t0,v0; div a2,a3; sw t0,4(a0); mfhi v1;
   jr ra; sw v1,0(a0)`.
  Note the store layout: the count store sits between `div` and `mfhi`
  (it needs no div latency slot), the putPos (remainder) store lands in the
  `jr ra` delay slot.

## Verification

- Candidate object slice: 17/17 words vs original at file 0x13C910
  (decomp-verifier, cross-checked against raw ELF bytes and the generated
  .s hex fields).
- Siblings readBufCreate/readBufDelete/readBufBeginPut/readBufEndGet
  re-verified unchanged in the same object.
- `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf` → byte-for-byte. `make split` moved the reference
  .s to `code/_generated/matchings/`. Count 754 → 753.

## Remaining in this file

- `readBufBeginGet__FP7ReadBufPPUc` (0x23B9D8, 0x48 bytes) still INCLUDE_ASM.
  Semantics already known: `if (count) *out = data + (putPos - count +
  capacity) % capacity; return count;` — same signed-modulo + guard idiom,
  return-load in the `jr ra` delay slot. Likely the next target; the int
  fields and the fuzzing harness carry over.
