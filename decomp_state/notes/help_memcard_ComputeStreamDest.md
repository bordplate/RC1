# memcard_ComputeStreamDest (code/game/help.cpp) — MATCHED 2026-10-07

`int memcard_ComputeStreamDest(unsigned int sectors, int* pDest1, int* pDest2)` at
vram `0x001FD6E0` (file offset `0xFE660`), 0x64 bytes. Computes the destination
addresses of a boot-asset CD stream read from its sector count.

## Identity / context

Splat attributes it to the help.cpp region purely by address. All five call
sites are memcard code: `memcard_Update` (0x2093D8) at four sites and
`func_00209370` at one. It sits between `memcard_GetName` (0x209030) and
`memcard_Update` (0x2093D8) in the generated memcard region.

The memcard save/restore state machine reads `bootAssets`'s second record size
into `sectors`, calls this to place the stream, reads it with
`snd_StreamSafeCdRead(lbn, sectors, *pDest1)`, then feeds
`*pDest1 + *(*pDest1 + 0x10)` to `memcard_RestoreGame`.

Logic:

```c
int memcard_ComputeStreamDest(unsigned int sectors, int* pDest1, int* pDest2) {
    if (sectors > MEMCARD_STREAM_MAX_SECTORS) {   // 0x20000
        *pDest1 = 0;
        *pDest2 = 0;
        return -1;
    }
    *pDest1 = levelMem.field_0x04 + currentVuChain - sectors;
    *pDest2 = levelMem.field_0x08 + currentVuChain - sectors;
    return 0;
}
```

`pDest2` is not currently consumed by any caller (written but the readback is
not used in the surviving callers).

## Globals

- `levelMem` @ 0x1940C0 — `LevelMem` struct. `field_0x04` / `field_0x08` are the
  two level-memory map-region bases. The struct was moved out of `bmain.cpp`
  into `code/include/levelmem.h` so help.cpp can share it; field names/offsets
  unchanged, only `field_0x04`/`field_0x08` used here. `InitMemSlots` (0x2015D8)
  fills `field_0x04 = D_24135F & 0xFFFFC000` and `field_0x08 = field_0x04 +
  *currentVuChain`, and extends the struct out to 0x28 (0x20/0x24/0x28 =
  0x7000000/0x7100000/0x7200000). The declared struct stays at its 0x1C extent;
  the trailing fields are still `field_XXXX`.
- `currentVuChain` @ 0x160F0C — plain `int`, in the GP window. Set by
  `vuChain_getCurrent`. Declared locally in help.cpp as `extern int` (mirrors
  the existing local decl in vuchain.cpp).

## Symbol naming

The original is C++ game code but the ELF is stripped, so the emitted symbol is
unmangled to match the `memcard_*` family and the generated label the callers
use. `config/symbols.txt` gained `memcard_ComputeStreamDest = 0x1fd6e0;` (Splat
uses it as the glabel + for the caller `jal`s), and the C++ definition carries
`asm("memcard_ComputeStreamDest")` so the mangled C++ name still resolves to the
unmangled label. Same documented pattern as `stream_breakTransitionCd`.

## Codegen notes

- The `sectors` parameter MUST be `unsigned int`. The original compares the
  count against 0x20000 with `sltu` (0x20001 does not fit an `sltiu` immediate,
  so EGC materializes 0x20000 via `lui` and tests `0x20000 < size`). A `signed
  int` emits `slt` and breaks the match.
- `levelMem` is read as a base register (`lui/addiu`) with `field_0x04` /
  `field_0x08` `lw`'d off it; `currentVuChain` loads self-based absolute twice
  (the store through `pDest1` defeats CSE of the second load).
- Plain form matches with default `-G8 -O2 -ffast-math -fno-exceptions -snas`;
  zero register pins, no per-TU flags, no address casts.

## Verification

- `decomp_probe.py` vs `func_001FD6E0.s`: 100/100 bytes, match.
- `tu_assembler_diff.py`: help.o 14/14, bmain.o 2/2 (struct move is
  codegen-neutral).
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf`: byte-identical.
- `decomp_status.py --count`: 571 -> 570.
