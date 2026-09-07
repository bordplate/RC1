# memcard_GetDataSize (0x20AC88, 56 bytes)

Matched 2026-09-07 with default flags, third probe attempt.

```c
extern "C" int memcard_GetDataSize(int* data) {
    int size = 8;
    while (data[0] != 0) {
        size += 8;
        size += data[1];
        size = (size + 3) & -4;
        data += 4;
    }
    return size + 8;
}
```

Purpose: total byte size of a memcard save block. `data` points at an array of
16-byte (4-int) record headers, each `{ header, size, ... }`, terminated by a
record whose first int is 0. The running total starts at 8; for every record it
adds the 8-byte header plus the record's `size` field, and re-aligns the total
down to a 4-byte boundary each pass (`(size + 3) & -4`). The function returns
`total + 8`.

Notes:
- Unmangled C symbol (`memcard_GetDataSize` in config/symbols.txt) sitting among
  C++-mangled siblings in the same file; `extern "C"` on the definition is the
  correct spelling (same as the matched `memcard_Init`).
- The 4-byte alignment is applied INSIDE the loop, not once after it. That is
  the crux: the `and $5, $2, $6` sits in the `bnez` branch delay slot, so it
  executes on every iteration, including the final one before the `jr $ra`.
  Ghidra's decompiler captured this correctly
  (`uVar3 = uVar3 + *piVar1 + 0xb & 0xfffffffc`), i.e. the `+3 & -4` folds into
  the per-iteration sum as `+ 0xb`.
- The mask constant `-4` (0xfffffffc) is loaded once, before the loop, into $6
  and held there across the whole loop; the `+3` is materialized per-iteration
  into $2 and ANDed in the delay slot. This is exactly what EGC emits when the
  rounding is written as a self-contained per-iteration statement
  `size = (size + 3) & -4;`.
- Statement ordering / splitting matters (fuzzed standalone):
  - v1: round once after the loop (`size += data[1] + 8;` in body,
    `size = (size+3) & 0xfffffffc;` after) -> wrong shape; EGC splits the mask
    constant into `li -65536; ori 0xfffc` and emits an 8-word body instead of
    14. Does not match.
  - v2: fold everything into one expression in the body
    (`size = (size + data[1] + 8 + 3) & -4;`) -> EGC hoists an `addu $2,$2,11`
    and keeps the accumulator in the wrong register. Same size, still a diff.
  - v3 (match): split the accumulation into `size += 8;` / `size += data[1];`
    then the standalone `size = (size + 3) & -4;`. This is the only form that
    reproduces the original's `addiu a5,a5,8; addu a5,a5,field; ...; addiu v0,
    a5,3; bnez; <and a5,v0,a6>` scheduling.
- Data pointer is a plain `int*` (32-bit); `data += 4` advances 16 bytes and the
  header/size fields are read at `data[0]`/`data[1]`. No struct type needed.
- Probe: decomp_state/probes/memcard_GetDataSize.cpp, all 56 bytes match
  (decomp_probe.py `match: true`, 0 differences).
- Full clean build + `cmp build/boot_elf.elf assets/boot_elf.elf` byte-for-byte.
  Nonmatching count 752 -> 751.
