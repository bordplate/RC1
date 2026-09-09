# videoDecAbort (code/game/movie/videodec.cpp)

- Original: 12 bytes (`0xC`) at file offset `0x13DBF0` in `assets/boot_elf.elf`,
  vram `0x0023CC70`. Object-relative `.text+0x1A8` in `videodec.o`
  (subsegment starts at file `0x13DA48` = videoDecCreate).
- Original instruction sequence:
  ```
  addiu   $v0, $zero, 1        # 24020001
  jr      $ra                  # 03e00008
    sw      $v0, 0xA8($a0)     # ac8200a8 (delay slot)
  ```
- Semantics: sets `VideoDec.state` (offset `0xA8`) to `1`. State values seen
  in siblings: reset=0, abort/started=1, flush-in-progress=2 (videoDecFlush),
  done=3 (videoDecMain ends with `videoDecSetState(self, 3)`).
- Single caller: `readMpeg__FP8VideoDecP7ReadBufP7StrFile` at vram `0x0023A588`
  (`jal videoDecAbort__FP8VideoDec`, a0=self); Ghidra shows one
  UNCONDITIONAL_CALL xref.

## EGC codegen quirk (important for future small setters)

Trial compiles with `-G8 -O2 -ffast-math -fno-exceptions`:

| candidate | result |
|---|---|
| `int f(VideoDec* s){ return (s->state = 1); }` | 4 instrs: `li v1,1; li v0,1; jr; sw v1` |
| `int f(VideoDec* s){ s->state = 1; return 1; }` | same 4-instr output |
| `int f(VideoDec* s){ s->state=1; return s->state; }` | same 4-instr output |
| `void f(VideoDec* s){ s->state = 1; }` | **3 instrs, exact match** (EGC reuses $v0 as free temp for the store) |

When a non-zero constant must both be stored and returned, EGC 2.95.2
duplicates the `addiu` into two registers ($v1 for the store, $v0 for the
return). A void setter keeps one register and matches original code that
happened to use $v0. (Zero constants are free in $zero, so `self->state = 0`
void/int both match — see videoDecReset.)

## Verification (mechanical)

- `make` builds; `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- `nm`/objdump of `build/code/game/movie/videodec.o`: symbol
  `videoDecAbort__FP8VideoDec` at `.text+0x1A8`, size `0xC`, words
  `24020001 03e00008 ac8200a8` (LE bytes `01000224 0800e003 a80082ac`) —
  identical to the original slice at file offset `0x13DBF0`.
- Plain C++ free function (no `extern "C"`) so the old-GCC mangled name
  matches Splat's call-site label.
- `python3 tools/decomp_status.py --count`: 881 -> 880.

## Next candidates in this file (same pattern)

- `videoDecSetState__FP8VideoDecUi` (12 bytes, vram `0x0023CC88`):
  `lw $v0,0xA8($a0); jr $ra; sw $a1,0xA8($a0)` — read-modify-write returning
  old value. Trial-compile `u32 f(VideoDec* s,u32 st){ u32 o=s->state;
  s->state=st; return o; }` first (no constant duplication involved).
- `videoDecInputCount__FP8VideoDec` / `videoDecBeginPut__...` /
  `videoDecEndPut__...` (0x1C each): pure forwarders to ViBuf methods at
  offset `0x48`; need a `ViBuf` type declaration and a method-bearing
  `VideoDec` layout (old-GCC mangling: free function, not class method —
  confirm label form in callers before writing C++ methods).
 - `videoDecIsFlushed__FP8VideoDec` (0x48): MATCHED 2026-09-09. See
   notes/videodec_videoDecIsFlushed__FP8VideoDec.md (the bnez delay slot
   zeroes $v0 so the not-flushed path returns 0; func_0012BA58 needs an
   unsigned return for the original `sltu`).
