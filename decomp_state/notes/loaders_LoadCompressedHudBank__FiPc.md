# LoadCompressedHudBank__FiPc (0x202D10, 0x64)

Matched 2026-10-10. Decompresses HUD bank `i` into `dest`, then clears that
bank's bankLoad marker.

## Semantics

```c
void LoadCompressedHudBank(int i, char* dest) {
    u32 aligned = ((u32)dest + 0xF) & ~0xF;   // dest rounded up to 16
    if (aligned != 0) {
        u32 base = (u32)(u8*)levelRoot;
        u32 idx = base + i * 8;
        u32 offset = *(u32*)(idx + 0x28);     // bank's compressed offset
        FastDecompress(offset + base, aligned);
    }
    u32* p = (u32*)((u8*)hudHeader + i * 4);
    p[0x1D] = 0;                              // *(header + i*4 + 0x74) = 0
}
```

- `dest` is `char*` (mangled `Pc`, not `PCc`): the second param is a plain
  pointer, not const char*.
- `FastDecompress(int source, int destination)` is `extern "C"` (bmain.cpp).
  source = levelRoot + offset, destination = the rounded dest.
- The `aligned` value is computed once and reused for both the `beqz` guard
  and the FastDecompress destination register (a1).

## Data symbols

- `levelRoot` (0x15EE4C, in the gp window). Declared in loaders.cpp WITHOUT the
  `.data` section attribute so that under -G8 EGC treats the 4-byte pointer as
  small-data-local and emits ONE bare pseudo `lw $4,levelRoot`; ps2eeas expands
  it in place to the original's self-based absolute `lui $4; lw $4`. With the
  `.data` attribute (as boot.cpp declares it) EGC instead emits a two-register
  split load (`lui $3; lw $4,off($3)`) that does not match. boot.cpp's only
  levelRoot use is inside ParseBin (INCLUDE_ASM, not compiled in the normal
  build), so the differing section attribute does not affect any compiled code.
- `hudHeader` (0x19A400) is a NEW symbol = `&hudHeap.header` (hudHeap 0x19A3E8
  + 0x18). The original reads the bank header as a direct symbol
  (`lui $2,%hi; lw $3,%lo` of 0x19A400); writing `hudHeap.header` (struct field)
  makes EGC emit a base+offset load (`lui hudHeap; addiu; lw 24`) that does not
  match. Added to config/symbols.txt.

## Match-sensitive constructs

- The address `base + i*8 + 0x28` must be SPLIT into `idx = base + i*8` then
  `*(u32*)(idx + 0x28)`. A single combined expression emits `addu $2,$2,$4`
  (v0 = i*8 + base) instead of the original `addu $2,$4,$2` (v0 = base + i*8);
  the separate `idx` step flips the addu operand order.
- The clear store is `p[0x1D] = 0` with `p = (u32*)(hudHeader + i*4)`: this keeps
  `i*4` in the base register and 0x74 as the store displacement
  (`sw $0,0x74($3)`), matching the original. (LinkHudBank in hud.cpp uses a
  different schedule, `(u32*)header + WORD_OFFSET` then `+ bank`; that form does
  NOT fit here.)
- The source is written `offset + base` (offset first), matching bmain.cpp's
  matched FastDecompress calls.

## Verification

Standalone probe (tools/decomp_probe.py) matches all 0x64 bytes. Full
`make clean && make split && make -j2` + `cmp build/boot_elf.elf
assets/boot_elf.elf` passes. decomp_status count 536 -> 535.
