# Help_TouchMessage (0x1FED30, 256 bytes) — matched 2026-10-08

Original: `func_001FED30` in `code/game/help.cpp`, plus a 4-byte dead tail
`func_001FEE30` immediately after it.

## Semantics

`extern "C" void Help_TouchMessage(int msgId)` — "touch" / move-to-end on the
active-message list (an LRU recency list of shown messages).

- `rec = messageIdCodeTableFind((s16)msgId, 0, 0)` (0x1FECC8): look up the
  record index whose **id** (column 0) equals `msgId`, or -1. The upper 16 bits
  of `msgId` are sign-extended (`sll`/`sra 16`) — the caller packs the id in the
  high half. Third arg (idOut) is 0.
- If `rec == -1`, return (id not in the table).
- Search the byte list `activeMessageRecords` (0x141E08, one `u8` per record
  index; count in `activeMessageRecordCount` at 0x15EE30) for `rec`.
- If found at index `i`: shift `recs[i..count-2]` left over `recs[i+1..count-1]`,
  zero the last slot, and decrement the count (removes the old copy).
- Always (both paths): append `rec` at `recs[count]` and increment the count.

Net effect: `rec` becomes the most recent entry. The list runs to 0x141EA0
(152 bytes, fits the 150 records of `messageIdCodeTable`); the pause/help
screen (consumer 0x21D168 in pause_post2.cpp) maps each stored index to its
code column and builds 12-byte sprite records from it, most-recent-last.

Sole boot-ELF caller: `mode_freezeInit` case 5 in `code/game/freeze.cpp`
(autosave warning). `reference/Lombyte` calls the same entry with one arg and
declares it C-style (`extern s32 func_001FED30()`; the s32/2-arg is Ghidra
over-inference — our matched build proves `void` + 1 arg).

## Data symbols (added to config/symbols.txt)

- `activeMessageRecords = 0x141e08;` — `extern u8 activeMessageRecords[];`
- `activeMessageRecordCount = 0x15ee30;` — `extern int activeMessageRecordCount;`
- `Help_TouchMessage = 0x1fed30;`

## Matching C form

```cpp
extern "C" void Help_TouchMessage(int msgId) {
    int rec = messageIdCodeTableFind((s16)msgId, 0, (u16*)0);
    if (rec == -1)
        return;
    int i = 0;
    for (; activeMessageRecords[i] != rec && i < activeMessageRecordCount; i++)
        ;
    if (i < activeMessageRecordCount) {
        while (i < activeMessageRecordCount - 1) {
            activeMessageRecords[i] = activeMessageRecords[i + 1];
            i++;
        }
        activeMessageRecords[i] = 0;
        activeMessageRecordCount = activeMessageRecordCount - 1;
    }
    int c = activeMessageRecordCount;
    activeMessageRecords[c] = (u8)rec;
    activeMessageRecordCount = c + 1;
}
```

## Codegen findings (EGC 2.95.2, `-G8 -O2 -ffast-math`, SN assembler)

Two independent, required shapes:

1. **Search = `for` loop, increment in the clause.** The original parks a
   *dead* count load (`lw v1, -32208(gp)`, GPREL) in the `blez a0` delay slot
   at 0x1FED74, plus an alignment nop at 0x1FED84. A `while` loop with `i++`
   in the body (candidate v15) reproduces everything *except* that dead load
   (49 diffs / 8-byte gap): EGC then puts the count *copy* (`move a2,a0`) in
   the delay slot instead of a second load. Only the `for` form (iterator
   increment in the clause) makes EGC emit the redundant hoisted loop-head
   count load. Register map: `rec`→a3, count→a0 (GPREL load in the `beq`
   delay), count copy→a2, `i`→a1, base hi→t1, base full→t0.

2. **Shift = `while` with `i++` in the body + direct array indexing.**
   `activeMessageRecords[i] = activeMessageRecords[i+1]` on the *global array*
   produces the original FORWARD loop. A pointer local (`u8* recs =
   activeMessageRecords;`) triggers EGC's **countdown transform** (`subu`) and
   a `for`-form shift re-pipelines it — candidates v3–v11 (pointer or for
   shift) all failed (55–59 diffs). Back-edge is plain `bnez v1`; the search
   back-edge is `bnezl v0` (Splat misdisplays it as `bnel`).

Minimal harness confirming the shift rule: array indexing → forward loop,
pointer local → countdown (see working scratch, `/tmp/opencode/shifttest/`).

## Dead tail (func_001FEE30)

4 bytes at 0x1FEE30: `addiu sp,sp,0x10` + alignment `nop` before the next
function. Ghost — no Ghidra function, `tools/deadness_scan.py` reports 0
references. Preserved with file-scope raw `asm(...)` (glabel
`func_001FEE30`) per the dead-tail policy; not a blocker.

## Verification

- Probe (`tools/decomp_probe.py`): 256/256, 0 diffs.
- decomp-verifier: region 0x1FED30–0x1FEE37 byte-identical (raw + objdump),
  full clean rebuild (`make clean && make split && make -j2`) parity OK.
- `python3 tools/decomp_status.py`: targets 563→561 (function + dead tail
  removed); neither is an active nonmatching target.
