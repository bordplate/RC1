# LoadDebugFont (code/game/bloaders.cpp) — MATCHED 2026-09-08

`void LoadDebugFont(void)` at vram `0x001E9338` (file offset `0xEA2B8`), 0x6C bytes
(plus the 8-byte dead orphan tail at 0x1E93A8, see below). Unmangled C symbol
(`extern "C"` in source; the probe rejects the C++-mangled `LoadDebugFont__Fv`).

## Identity / context

Loads the debug font texture and builds its PIF header. Logic:

```c
extern "C" void LoadDebugFont(void) {
    u8 buf[0x18];
    Load(D_001AABC0, D_00137B80.src, D_00137B80.size);
    LoadPifAsPSMT8H(D_001AABC0, buf, *(u32*)0x15EE88 + 0xC0000, 0x3FFC00);
    *(u64*)0x15EEC8 = *(u64*)buf;
}
```

- `Load` (0x216828, code/game/stream.cpp, still INCLUDE_ASM):
  `int Load(u8* dst, int src, int size)` — pump-loop file load; returns bytes
  written. Same (src,size) table pattern as the six Load calls inside
  Transition_DoTransition (0x1EB798) that read descriptor pairs at 0x139078+.
- `LoadPifAsPSMT8H` (0x1E9168, same file, still INCLUDE_ASM):
  `void LoadPifAsPSMT8H(u8* src, u8* out, int base, int mask)` — GIF-pixmaps the
  texture (width from src+8, height from src+0xC, 0x1B = 27bpp-ish params,
  0x137B80-style descriptor at src) and writes a 0x18-byte PIF header to `out`:
  `[0]` = 64-bit PIF tag, `[1]` = 1, `[2]` = 0.
- Sole caller: 0x201958 (Ghidra FUN_00201650 region, level/menu init path).

Globals:

- `D_00137B80` (core.data, zero at boot): load descriptor
  `{ pad[8], u32 src @0x137B88, u32 size @0x137B8C }`. Declared as a struct with
  `__attribute__((section(".data")))`; the two field loads share the
  `lui v0,%hi; addiu v0,%lo` base in the original (0x137B80 + 8/12).
- `D_001AABC0` (.data, zero at boot): the debug-font destination buffer, passed
  by ADDRESS to both calls (see codegen finding below).
- `D_0015EE88` (core.lit): base used as `*(u32*)0x15EE88 + 0xC0000` (third arg of
  LoadPifAsPSMT8H). In-window, so constant-cast for the original's absolute
  `lui v1,0x16; lw v1,-4472(v1)`.
- `D_0015EEC8` (core.lit): receives the first 8 bytes of the PIF header
  (`ld v0,0(sp); lui at,0x16; sd v0,-4408(at)`).

## Codegen findings (the s0 question)

The original keeps 0x1AABC0 in `s0` (sq/lq + `move a0,s0` before each call).
EGC 2.95.2 only reproduces that when the value is a SYMBOL ADDRESS:

| form tested (probes v2/v3/v4) | result |
|---|---|
| literal `0x1AABC0` in both calls | re-materialized `lui a0,0x1a; ori a0,a0,0xabc0` per use; no s0; 0x30 frame; 24 diffs |
| `int dst = 0x1AABC0;` local | constant-folded, identical to literal; 24 diffs |
| `extern "C" u8 D_001AABC0[]` passed directly | **match** — symbol address survives CSE as one value live across the first call, so RA assigns s0 |

The object file shows this concretely: the candidate emits `%hi/%lo`
relocations against `D_001AABC0` (not constants), which the linker resolves to
the original's `lui s0,0x1b; addiu s0,s0,-0x5440` words.

Other facts:

- Argument loads for the first call come out `lw a2,12(v0); lw a1,8(v0)`
  (a2 first) with the second load in the `jal` delay slot — as in the original,
  with the struct-field form.
- `buf[0x18]` and `buf[0x20]` both match byte-for-byte (EGC pads the local area
  to a 0x20 span; LoadPifAsPSMT8H writes exactly 0x18 bytes, so 0x18 is the
  faithful size).
- Second-call setup sequence matches with default flags: `lui v1; lw v1; lui a2;
  lui a3; move a0,s0; addu a2,v1,a2; move a1,sp; jal; <ori a3,a3,0xfc00>`.
- The 4-byte nop gap at 0x1E93A4 (between the function end 0x1E93A4 and the
  tail at 0x1E93A8) is linker fill — the .ld pins both symbols absolutely.

## The dead tail (func_001E93A8) — EGC artifact, BLOCKED

After the epilogue the original contains unreachable bytes:

```
0x1E939C: jr ra
0x1E93A0: addiu sp,sp,0x40     (delay slot; function ends at 0x1E93A4)
0x1E93A4: nop                  (gap, linker fill in the build)
0x1E93A8: addiu sp,sp,0x10     <- unreachable
0x1E93AC: nop
```

Unlike the sibling tails at 0x12EC00 (snd_StopAllStreams) and 0x23ACB0
(audioDecDelete) — which are exact duplicates of the parent's 0x10 deallocate —
this one is a 0x10 deallocate under a parent whose frame is 0x40. Nothing can
jump to it: raw-encoding scan of the whole boot ELF finds no `jal`
(0x0C07A4EA) or `j` (0x0807A4EA) targeting 0x1E93A8, and Ghidra has no
function there.

No C form regenerates it: a local accounting for 0x10 inflates the parent frame
to 0x50 (prologue `-0x40`/epilogue `+0x40` are verified in the binary), and
local EGC emits no dead instruction after `jr ra` in any tested form.
last-resort-decompiler (GPT-5.6 Sol) tested the dead volatile-local variant
(first diff at 0x1E9338: frame -0x50) and confirmed the dead-tail probes
(t1-t10) show no post-delay-slot dead code from this EGC; verdict: retain the
orphan INCLUDE_ASM, which supplies the 8 bytes and keeps parity.

## Verification

- Probe (v5, buf[0x18]): `candidate.json` match:true, 108/108 bytes, zero
  differences.
- Built object `build/code/game/bloaders.o`: `LoadDebugFont` 27 words; differs
  from the original only in the 6 pending-relocation fields
  (HI16/LO16 D_00137B80, HI16/LO16 D_001AABC0, 2x jal R_MIPS_26) — resolved
  correctly at link.
- `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf`: byte-for-byte.
- `python -m unittest discover -s tools -p 'test_decomp_*.py'`: 11 OK.
- Count 746 -> 745 (tail INCLUDE_ASM retained + blocked with note).
