# msg_string__Fi (code/game/help.cpp) — MATCHED 2026-09-05

`char *msg_string(int idx)` at vram `0x001FDD10` (file offset `0xFEC90`), 0x40 bytes
(plus 0x8 bytes of dead tail, see below). Returns the text of help message `idx`.

## Identity / context

Callees: `Help_FindIndex(int)` (0x1FDCA0, still INCLUDE_ASM, unmangled C symbol in the
binary — declare `extern "C"`). ~110 unconditional xrefs, mostly moby dialogue code
(0x21xxxx) plus a few in draw/help paths; callers pass the result straight to
`FUN_001FF658` (text blit).

Logic:

```c
int i = Help_FindIndex(idx);      // index of msg id, or -1
if (i >= 0)
    return HelpMsgs[i].text;      // 16-byte-stride entry array
return s_Paradox_this_message_does_not;  // "Paradox: This message does not exist" @ 0x199968
```

Globals:

- `HelpMsgs` @ 0x15F6A0 — pointer global (in the .lit region, pinned in the .ld).
  Its VALUE is the base of an array of **16-byte** entries; entry+0 = `char *text`,
  entry+4 = message id (what Help_FindIndex compares against). NOT a `char **`
  (that miscompiles to `sll v1, v1, 2`).
- `s_Paradox_this_message_does_not` @ 0x199968 — plain string in .data (writable
  segment), 36 bytes.

## Replacement (code/game/help.cpp)

```cpp
extern "C" int Help_FindIndex(int idx);

struct HelpMsg {
    char *text;
    int id;
    int f8;
    int fC;
};

extern struct HelpMsg *HelpMsgs;
extern char s_Paradox_this_message_does_not[];

char *msg_string(int idx) {
    int i = Help_FindIndex(idx);
    if (i >= 0)
        return HelpMsgs[i].text;
    return s_Paradox_this_message_does_not;
}
```

EGC old-cfront mangling of the free function gives exactly `msg_string__Fi`.

## Codegen facts that matter

- The **early-return form (no local holding the result) is required**. Both branches
  then write the result straight to `v0`, EGC emits `bgezl v1` -> then path with the
  else (`lui/addiu` of the string address + `b` to the epilogue) on fallthrough, and
  one shared epilogue `lq ra; jr ra; addiu sp, sp, 0x10`. The `HelpMsgs` pointer load
  lands in the `bgezl` delay slot.
- The `char *s` local form (with or without a trailing store) miscompiles: EGC flips
  the branch to `bltz`, keeps the result in `v1`, and adds a `move v0, v1` before the
  epilogue.
- `HelpMsgs[i].text` with the 16-byte struct gives `sll v1, v1, 4; addu; lw v0, 0(v1)`
  exactly; `char **` gives shift 2 and breaks parity at 0x1FDD38 (sll sa field).
- `extern "C"` on `Help_FindIndex` is required (unmangled symbol in the binary).
- The string must be declared non-`const` (it lives in .data); a const declaration
  makes EGC error out on the qualifier-discard in this toolchain.

## The dead tail (func_001FDD50) — EGC artifact

After the epilogue the original contains **unreachable dead code**:

```
0x1FDD48: jr ra
0x1FDD4C: addiu sp, sp, 0x10    (delay slot)
0x1FDD50: sh zero, 0x38(v0)     <- unreachable
0x1FDD54: nop
```

Reachability was checked exhaustively: no `jal` or `j` to 0x1FDD50 anywhere in
core.text/.text/overlays (raw-encoding search), no absolute word refs, and the only
computed branch in the neighborhood (func_001FDC08, jumptable `jtbl_001E7A20`) has
entries only in 0x1FDCxx–0x1FE7xx. Ghidra has no function at 0x1FDD50.

The matched candidate C does NOT regenerate these bytes (EGC emits a clean 0x40-byte
function), so the orphan `INCLUDE_ASM(..., func_001FDD50)` is **retained** to supply
the 8 bytes; the function and its dead tail both compare equal, full ELF parity OK.

### Empirical dead-store behavior (from standalone EGC tests)

With two global stores before a `return`, EGC puts the FIRST store alive (right
before `jr ra`) and the SECOND store **dead, after** the `jr ra`:

```c
c = c + 0x10;
D_A = c;      // alive, before jr
D_B = c;      // dead, after jr
return c;
```

reproduces exactly `...; sw; jr ra; sw(dead)`. This is the mechanism that produced
the dead tails in the binary. For msg_string no C form found so far reproduces the
single dead `sh zero, 0x38(v0)` while keeping the matched body — tried:
early return (no dead bytes), local + 1 store (store alive + move), local + struct
store (same). If a form is found that regenerates the tail, the orphan INCLUDE_ASM
can be dropped (the .ld pins would then just define the symbols absolutely).

### Systemic: ~46 dead-tail "functions"

The same artifact appears ~46 times across the binary as 4-byte spimdis "functions"
named `func_XXXX` (all in the queue). Contents observed: `addiu sp, sp, N` (stack
deallocate, ~28×), `sw/sh` of a return value or arg field (e.g. the four
`sw v0, -0x3FE0(gp)` in vuchain after functions that end with `jr ra; sw v0, X(gp)`
in the delay slot — i.e. the C had two consecutive stores of the same value and EGC
kept one in the delay slot and dropped the other after the return), `lw v0, off(a1)`,
`daddu v0, t9, 0`, `andi a3, v0, 0xFF`, bare `nop`s. They are all unreachable tails
of the PRECEDING function, at 8-byte-aligned addresses right after a `jr ra` epilogue.

Strategy: when decompiling the preceding function, test whether the right C form
makes EGC regenerate the tail bytes; if yes, drop the orphan INCLUDE_ASM in the same
commit. If no, keep the orphan INCLUDE_ASM (it supplies the bytes) and block the
orphan entry with this explanation.

## Verification

- Candidate object slice == original 0x40 bytes (shift-amount diff found and fixed
  the `char **` vs 16-byte-struct mistake: build had `sll v1, v1, 2` at 0x1FDD38).
- `make` clean + `cmp build/boot_elf.elf assets/boot_elf.elf` → byte-for-byte.
- Count 818 -> 817 (func_001FDD50 now blocked with note).
