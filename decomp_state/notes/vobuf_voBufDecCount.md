# voBufDecCount__FP5VoBuf (code/game/movie/vobuf.cpp)

- Original: 32 bytes (`0x20`) at file offset `0x13E2C0` in `assets/boot_elf.elf`,
  vram `0x0023D340`. Object-relative `.text+0x1B0` in `vobuf.o`.
- Semantics: ring-buffer consumer decrement — `if (count > 0) count--;` on the
  `VoBuf` (`count` at offset `0xC`).
- Original instruction sequence:
  ```
  lw     $v0, 0xC($a0)   # load count
  blez   $v0, .Lend      # if count <= 0 skip  (SIGNED test!)
     nop
  lw     $v0, 0xC($a0)   # reload (volatile)
  addiu  $v0, $v0, -1
  sw     $v0, 0xC($a0)   # count--
  .Lend:
  jr     $ra
     nop
  ```

## The signed/unsigned trap

Ghidra `FUN_0023d340` decompiles to `if (0 < *(int*)(p+0xc)) *(int*)(p+0xc) -= 1;`
and the sole xref is an UNCONDITIONAL_CALL from vram `0x0023b564` — a video
IRQ handler (`EI(); SYNC(0);` epilogue) that ignores the return value, so the
function is `void`.

The branch opcode is the tell: the original uses **`blez`** (signed <= 0).
Standalone test compiles with identical EGC flags showed:

```cpp
volatile u32 count;  if (self->count > 0) ...   // -> beqz $v0, .Lend  (unsigned idiom)
volatile int count;  if (self->count > 0) ...   // -> blez $v0, .Lend  (MATCHES)
```

So the original source declared `count` as a **signed** counter even though it
is conceptually unsigned. This forced a one-word struct fix in vobuf.cpp:
`volatile u32 count` -> `volatile int count`. No layout change (both 4 bytes,
offsets identical), and no codegen impact on the already-matched siblings —
`voBufIsEmpty` (`== 0` -> `sltiu x,1`) and `voBufIsFull` (`==` -> `xor`+`sltiu`)
are sign-insensitive equality tests; full-ELF parity confirms it.

## C candidate (old-GCC mangling, per voBufDelete note)

```cpp
void voBufDecCount(VoBuf* self) {
    if (self->count > 0) {
        self->count--;
    }
}
```

Plain C++ free function; EGC 2.95.2's old-GCC ABI mangles it to exactly
`voBufDecCount__FP5VoBuf` (verified empirically via `nm`). No `extern "C"`,
no manual mangling. `volatile` forces the second load inside the branch,
reproducing the original's load/test/reload/store shape; `-O2` does not
reorder anything else (8 words, no delay-slot trickery).

## Verification (mechanical)

- Standalone EGC test compile (`/tmp/opencode/vbtest2.cpp`, project flags
  `-G8 -O2 -ffast-math -fno-exceptions -Wa,-EL`) produced bytes identical to
  the original slice at file offset `0x13E2C0`:
  `0c00828c 04004018 00000000 0c00828c ffff4224 0c0082ac 0800e003 00000000`
  (byte-by-byte compare: MATCH, before touching the source).
- `make` builds; `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- `objdump -d build/code/game/movie/vobuf.o`: symbol
  `voBufDecCount__FP5VoBuf` at `.text+0x1B0`, size `0x20`, same words as above.
- `objdump -r build/code/game/movie/vobuf.o`: no relocations in `[0x1B0,0x1D0)`;
  only the pre-existing R_MIPS_26 records at 0x8C/0xE0 (voBufIncCount's
  func_0011D660/func_0011D6A8 calls) and 0x104/0x154 remain.
- `decomp_status.py --count` dropped 886 -> 885.

## Remaining vobuf.cpp targets (same family)

- `voBufIncCount__FP5VoBuf` — the interesting one: writes a const (`lui/ori
  0x138C0`) through `tags*const + head`, increments count, guards
  `capacity == 0` with `beql -> break 7`, then `head = (head+1) / 2` using
  unsigned `div`. The signed `count` vs unsigned `head` mix now visible in the
  struct is consistent with this asm (`div` not `dsdiv` on head). Needs the
  identity of func_0011D660/func_0011D6A8 (look like lock/unlock) and the
  meaning of the 0x138C0 constant before a C candidate is sane.
- `voBufGetData__FP5VoBuf` / `voBufGetTag__FP5VoBuf` — the pop pair around
  vram `0x23D2D8`; data one has a capacity guard (`beqzl -> break 7`).
- `voBufCreate__FP5VoBufP6VoDataP5VoTagi` — needs the allocator/global VoData
  + VoTag symbols.
