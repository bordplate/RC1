# Help_FindIndex (0x1FDCA0, 0x6C) — MATCHED (2026-10-07)

Function: `code/game/help.cpp` : `Help_FindIndex` (VRAM 0x1FDCA0, size 0x6C, 27 words).
C linkage; the level-overlay callers reference the original unmangled entry point,
so the definition keeps `extern "C"`.

## Semantics

```c
extern "C" int Help_FindIndex(int idx) {
    int ret = -1;
    int i = 0;
    while (i < g_helpState.msgCount) {
        if (HelpMsgs[i].id == idx) {
            ret = i;
            break;
        }
        i++;
    }
    return ret;
}
```

Returns the index of the first `HelpMsgs[i]` whose `id` equals `idx`, or -1 when
no messages are loaded (`g_helpState.msgCount <= 0`) or none match.
`HelpMsgs` (0x15F6A0) points at a 16-byte `HelpMsg` array (id at +0x4);
`g_helpState.msgCount` is the +0x2C member (the `HelpMsgCount` symbol, 0x1996FC).

## Codegen notes (EGC 2.95.2, default flags, first probe)

The natural while/break form matched on the first standalone probe, no pins or
flags needed. Observable EGC behavior worth keeping in mind:

1. **First-iteration peel.** EGC peels the i=0 iteration out of the loop: the
   precheck loads `HelpMsgs[0].id` with the raw base (no `i*16` math, the
   constant-folded subscript) and on a match sets `ret = 0` in the delay slot
   of an unconditional `b` to the shared exit.
2. **Increment at loop top.** The remaining loop body starts with `i++`, then
   the `i < count` test (signed `slt`), then the id compare whose mismatch
   branches back to the `i++`. Per iteration the sequence is
   `i++; count check; id check`.
3. **Per-iteration global reload.** `g_helpState.msgCount` is reloaded from the
   global every iteration (no CSE out of the loop); the base hi page is kept
   in t0 (`lui v0; daddu t0,v0,0; addiu v0,v0,%lo`) and the loop re-derives
   the base with `addiu v1,t0,%lo` — one copy on the fall-through plus a
   duplicate in the taken-delay slot of the back-edge `bne`.
4. **Single shared exit.** `ret` (a2) is set on every path (-1 in the
   prologue, 0 and i on the match paths) and the one exit does
   `jr ra; daddu v0,a2` — the multi-path-local + single-return structure,
   not multiple return statements.
5. **HelpMsgs load.** Plain `extern struct HelpMsg* HelpMsgs;` (in GP window)
   compiles under default `-G8` to the bare small-data pseudo that ps2eeas
   expands to the original's self-based `lui v1; lw v1,%lo(v1)` pair.

## Related changes

- `struct HelpState` gained `field_0x28` and `msgCount` (+0x2C) so the count
  is a named member instead of an opaque offset. No other TU declares the
  struct. `config/symbols.txt` unchanged (g_helpState 0x1996D0, HelpMsgs
  0x15F6A0, HelpMsgCount 0x1996FC already present).
- The `HelpMsg` struct and `HelpMsgs` / `s_Paradox_this_message_does_not`
  externs moved above the definition (declarations only; no layout effect).

Verification: standalone probe 108/108 bytes, 0 diffs; full clean rebuild
(`make clean && make split && make -j2`) + `cmp build/boot_elf.elf
assets/boot_elf.elf` byte-identical; `decomp_status.py --count` 567 -> 566.
