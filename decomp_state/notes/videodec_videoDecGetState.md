# videoDecGetState (code/game/movie/videodec.cpp)

- Original: 8 bytes (`0x8`) at file offset `0x13DC00` in `assets/boot_elf.elf`,
  vram `0x0023CC80`. Object-relative `.text+0x1b8` in `videodec.o`.
- Original instruction sequence:
  ```
  jr      $ra              # 03e00008
    lw      $v0, 0xA8($a0) # 8c8200a8 (delay slot)
  ```
- Ghidra decompile of `FUN_0023cc80`: `return *(undefined4 *)(param_1 + 0xa8);`
  — a plain int getter of the field at offset `0xA8`.
- Field identity: sibling `videoDecSetState__FP8VideoDecUi` (file offset
  `0x13DC08`) does `lw $v0,0xA8($a0); jr $ra; sw $a1,0xA8($a0)` — writes the
  new state into the same word and returns the old value, confirming `0xA8` is
  an int state member. Callers are absolute `jal 0x23cc80` from videodec's own
  range (vram `0x23A690`, `0x23A6F8`, `0x23CE90`, `0x23D018`; Ghidra shows all
  four as UNCONDITIONAL_CALL), so no symbol resolution is needed for this
  change.

## C candidate

`videodec.cpp` had no type definition, so a local struct was added (same
pattern as the `VoBuf` struct in `vobuf.cpp`):

```cpp
typedef struct VideoDec {
    u8 _pad[0xA8];
    u32 state;
} VideoDec;

extern "C" int videoDecGetState(VideoDec* self) {
    return self->state;
}
```

(`#include "types.h"` was added for `u8`/`u32`; `common.h` does not pull it in.)

Only one memory op, nothing for the scheduler to reorder. EGC 2.95.2 `-O2`
emits exactly `jr $ra; lw $v0, 0xA8($a0)` — standalone test compile confirmed
the words before touching the source.

## Verification (mechanical)

- `make` builds; `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- `objdump -d build/code/game/movie/videodec.o`: symbol `videoDecGetState` at
  `.text+0x1b8`, size `0x8`, words `03e00008 8c8200a8` (little-endian bytes
  `0800e003 a800828c`) — identical to the original slice at file offset
  `0x13DC00`.
- No relocations at `.text+0x1b8`/`.text+0x1bc` in `videodec.o`.
- `python3 tools/decomp_status.py --count`: 884 -> 883.

## Next candidates in this file (same pattern)

- `videoDecReset__FP8VideoDec` (8 bytes, file offset `0x13DBB0`, vram
  `0x23CC30`): `jr $ra; sw $zero, 0xA8($a0)` — expected C: `self->state = 0;`.
- `videoDecSetState__FP8VideoDecUi` (12 bytes): expected C:
  `u32 old = self->state; self->state = state; return old;`
