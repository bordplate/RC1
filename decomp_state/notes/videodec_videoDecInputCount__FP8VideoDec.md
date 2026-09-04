# videoDecInputCount (code/game/movie/videodec.cpp)

- Original: 28 bytes (`0x1C`) at file offset `0x13DC60` in
  `assets/boot_elf.elf`, vram `0x0023CCE0`. Object `.text+0x218` in
  `videodec.o`. Original instruction sequence:
  ```
  addiu   $sp, $sp, -0x10
  sq      $ra, 0($sp)
  jal     viBufCount__FP5ViBuf
    addiu $a0, $a0, 0x48          # (delay slot: arg = self + 0x48)
  lq      $ra, 0($sp)
  jr      $ra
    addiu $sp, $sp, 0x10          # (delay slot)
  ```
- Semantics: `return viBufCount(&self->vibuf);` — one-arg wrapper passing
  the VideoDec-embedded ViBuf at offset `0x48`. Sole caller is
  videoDecIsFlushed (vram `0x23CDE0`, unconditional call at `0x23CDF4` with
  self still in `$a0`; result compared against 0).

## VideoDec / ViBuf layout recovered (Ghidra)

videoDecCreate (`0x23CAC8`) calls, in order:
`sceMpegInit`-lib (`0x12b7d8`), five `0x12bb10(self, id, handler, 0)`
callback registrations (ids 0,1,2,3,5 -> `mpegError@0x23D080`,
`mpegNodata@0x23D0A8`, `func_0023D0E0`, `func_0023D110`,
`func_0023D140`), then videoDecReset (`0x23CC30`, the matched 2-word
`sw zero,168(a0); jr ra` — it is a real function, not gap bytes), then
`viBufCreate(self + 0x48, ...)` and returns 1. videoDecDelete calls
`viBufDelete(self + 0x48)`. So VideoDec embeds a ViBuf object at `0x48`;
`state` at `0xA8` follows immediately, fixing sizeof(ViBuf) = `0x60`
(first draft used 0x58 and shifted state to 0xA0 — caught by full cmp).

ViBuf offsets (from viBufCreate/viBufReset/BeginPut/Count/GetTs/PutTs/
ModifyPts/StopDMA/RestartDMA; StopDMA saves IPU/DMAC register state into
`0x1C..0x3C`, which is why that block exists):

| off  | meaning (evidence)                                   |
|------|------------------------------------------------------|
| 0x00 | DMA data base, masked & 0xfffffff at use             |
| 0x04 | IPU CHCR descriptor, create writes v & 0xfffffff \| 0x20000000 |
| 0x08 | number of 0x800-byte blocks                          |
| 0x0C/0x10/0x14 | position fields (reset zeroes all three)     |
| 0x18 | capacity in bytes = blocks << 11                     |
| 0x1C..0x3C | saved dmac4-to / dmac3-from / IPU BP+CTRL regs (StopDMA/RestartDMA) |
| 0x40 | semaphore (WaitSema/SignalSema everywhere, create CreateSema) |
| 0x44 | DMA active flag (reset sets 1, StopDMA clears)       |
| 0x48/0x4C | zeroed by create (`sd $0, 0x48`)                   |
| 0x50 | tag array base (0x18-byte entries, reset fills -1/-1/0/0 per entry) |
| 0x54 | tag count (reset loop bound, GetTs modulo)           |
| 0x58 | used-tag count (GetTs loop bound, reset zeroes)      |
| 0x5C | tag put index (GetTs ring start, reset zeroes)       |

The videodec.cpp `ViBuf` typedef mirrors this table; the only codegen
constraint for THIS function is that `&self->vibuf == self + 0x48`, so any
later field renaming is safe as long as offsets/size are preserved.

## Replacement

```cpp
int viBufCount(ViBuf* buf);   // mangles to viBufCount__FP5ViBuf (cfront free-fn recipe)

int videoDecInputCount(VideoDec* self) {
    return viBufCount(&self->vibuf);
}
```

Plain C++ free function; EGC 2.95.2 old-ABI mangling of `(VideoDec*)` and
`(ViBuf*)` reproduces both exact Splat labels. No extern "C" needed.

EGC codegen note: with the single argument being `base + small_offset`,
EGC emits the `addiu $a0,$a0,0x48` directly in the `jal` delay slot
(identical to how videoDecDelete's original puts `addiu a0,s0,72` after its
`jal viBufDelete`) — no separate setup word, matching the 7-word body.

## Verification (mechanical)

- `make` builds; `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- objdump of `[0x23CCE0, 0x23CCFC)` in build vs original ELF: all 7 words
  identical (`27bdfff0 7fbf0000 0c08f184 24840048 7bbf0000 03e00008
  27bd0010`).
- `objdump -t videodec.o`: `videoDecInputCount__FP8VideoDec` at `.text+0x218`,
  size `0x1C`.
- `make split` re-runs cleanly and Splat moved
  `videoDecInputCount__FP8VideoDec.s` from `_generated/nonmatchings/...` to
  `_generated/matchings/game/movie/videodec/`.
- `python3 tools/decomp_status.py --count`: 835 -> 834.
