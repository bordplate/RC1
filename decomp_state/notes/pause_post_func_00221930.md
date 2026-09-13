# pause_drawRangeList (func_00221930) — 0x00221930, 56 bytes

Matched 2026-09-13. `code/game/pause_post.cpp`.

## What it does

Pause-menu callback invoked through the function-pointer table entry at
0x1D2264 (the only non-null neighbors are 0x221908 and this slot; there are no
direct `jal`/`j` references in the boot image). It takes a pointer to a menu
object that carries two element ranges and forwards them, as (start, end)
pairs, to the shared list drawer `func_001FD748` (still `INCLUDE_ASM` in
`help.cpp`), then returns the constant 2.

The menu object layout is confirmed only through offset 0x27 (the tail the
function touches); the preceding 0x18 bytes are padding in the local struct:

```
0x18  base1   start of range 1
0x1C  base2   start of range 2
0x20  count1  element count of range 1
0x24  count2  element count of range 2
```

The call lowers to `drawer(base1, base1+count1, base2, base2+count2)`. The
drawer computes `(end - start) * 0x10` for each range, so `countN` is a number
of 0x10-byte elements, not a byte length. (The callee draws a 19-slot list,
highlighting the current entry, and ends with `DoGifPaging__Fv`.)

## C form (matched)

```cpp
typedef struct {
    u8 pad[0x18];
    u32 base1;
    u32 base2;
    u32 count1;
    u32 count2;
} PauseRangeList;

int pause_drawRangeList(PauseRangeList* list) asm("func_00221930");
int pause_drawRangeList(PauseRangeList* list) {
    help_drawRangeList(list->base1, list->base1 + list->count1,
                       list->base2, list->base2 + list->count2);
    return 2;
}
```

`help_drawRangeList` is a local `extern` prototype with an
`asm("func_001FD748")` label; the callee's ELF symbol is the unmangled address
placeholder `func_001FD748`. The callee is `void` returning four `int`
arguments (its epilogue sets no return value), and the ignored void return does
not affect the caller's allocation here.

## Verification

- `decomp_probe.py` (default `-G8 -O2` flags, `--define func_001FD748=0x001FD748`)
  matched all 56 bytes on the first attempt, zero differences.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes
  byte-for-byte. Nonmatching count 704 -> 703.

## Codegen notes

No special flags or register pins needed. EGC emits the 0x10 frame
(`sq ra,0(sp)` after the `v0=a0` base move), loads the four `u32` fields into
a0/a2/a3/a1, folds `a3+=a2` before the `jal` and `a1+=a0` into the `jal` delay
slot, and returns the hoisted `addiu v0,$0,2`. The plain `u32` fields (not
pointers) are what keep `base+count` a raw `addu` rather than scaled pointer
arithmetic.
