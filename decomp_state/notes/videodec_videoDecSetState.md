# videoDecSetState (code/game/movie/videodec.cpp)

- Original: 12 bytes (`0xC`) at file offset `0x13DC08` in
  `assets/boot_elf.elf`, vram `0x0023CC88`. Object-relative `.text+0x1C0`
  in `videodec.o`. Original instruction sequence:
  ```
  lw      $v0, 0xA8($a0)        # 8c8200a8
  jr      $ra                   # 03e00008
    sw      $a1, 0xA8($a0)      # ac8500a8 (delay slot)
  ```
- Layout neighbours (original vram): videoDecGetState ends at `0x23CC88`,
  this function ends at `0x23CC94`, then a 1-word nop gap (`0x23CC94`),
  then videoDecPutTs at `0x23CC98`. The trailing nop belongs to the
  following INCLUDE_ASM blob and is untouched by this change.
- Semantics: read-modify-write of `VideoDec.state` (offset `0xA8`) —
  stores the new state, returns the previous one. State values per
  videoDecAbort note: reset=0, abort/started=1, flush-in-progress=2,
  done=3 (`videoDecMain` ends with `videoDecSetState(self, 3)`).
- Replacement (plain C++ free function; EGC 2.95.2 old-ABI mangling of
  `(VideoDec*, u32)` reproduces the exact Splat label):
  ```cpp
  u32 videoDecSetState(VideoDec* self, u32 state) {
      u32 old = self->state;
      self->state = state;
      return old;
  }
  ```
- EGC codegen note: `u32 old = self->state;` loads directly into `$v0`
  (the return register is free, so no extra move), the store keeps its
  argument in `$a1`, and the store falls into the `jr $ra` delay slot —
  exactly the original 3-instruction shape. No constant duplication
  involved (unlike the videoDecAbort quirk).

## Verification (mechanical)

- `make` builds; `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- `objdump -t build/code/game/movie/videodec.o`:
  `videoDecSetState__FP8VideoDecUi` at `.text+0x1C0`, size `0xC`;
  object bytes there = `a800828c 0800e003 a80085ac`, identical to the
  original slice at file offset `0x13DC08`.
- `objdump -r` shows no `.rel.text` relocations inside `[0x1C0, 0x1CC)`
  (only register/offset ops, no symbols), so raw object bytes equal the
  linked output. Neighbours contiguous as original: videoDecGetState
  ends at `0x1C0`, videoDecPutTs starts at `0x1D0` (the `0x1CC` word is
  the pre-existing inter-function nop from the PutTs blob).
- `python3 tools/decomp_status.py --count`: 871 -> 870.
