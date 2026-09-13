# ResetGsRegistersPr__Fv (0x001F3958, 0x78 bytes)

Matched 2026-09-13.

## Function
Resets the GS (Graphics Synthesizer) display registers, which the EE maps at
0x12000000. Writes eight 64-bit register slots in this order (matching the
store sequence):

| store | GS register (EER map) | value |
|-------|------------------------|-------|
| 1 | BGCOLOR  0x120000E0 | 0 |
| 2 | PMODE    0x12000000 | 0xFFA1 |
| 3 | SMODE2   0x12000020 | gsDisplaySettings.smode2 |
| 4 | DISPFB1  0x12000070 | gsDisplaySettings.dispfb |
| 5 | DISPFB2  0x12000090 | gsDisplaySettings.dispfb |
| 6 | DISPLAY1 0x12000080 | gsDisplaySettings.display |
| 7 | DISPLAY2 0x120000A0 | gsDisplaySettings.display |
| 8 | EXTWRITE 0x120000D0 | 0 |

The three struct values come from `gsDisplaySettings` @ 0x151788 (renamed from
D_00151788 in config/symbols.txt), a 24-byte zero-initialized global (three
u64: smode2, dispfb, display). It is only READ in the boot image (raw scan for
any instruction using the 0x150000 hi-page + 0x1788/0x1790/0x1798 lo found
only this function's `ld`/`addiu` pair); its writer, if any, is level-overlay
code, so the fields stay zero for boot parity. PMODE 0xFFA1 = FIX 255 |
alpha | color | PT0 (polygon) — a typical primitive-mode reset, hence the
function's "Pr" suffix.

Caller: the space-level draw loop at 0x231BD8 (calls ResetGsRegisters, builds
VU1 packets, draws textured quads, then calls ResetGsRegistersPr each
iteration).

## Key codegen finding: volatile constant-address casts required
A plain `*(u64*)0x120000XX = ...` cast lets EGC (2.95.2, -O2) recognize that
all eight addresses share the 0x1200 hi-page and fold every store onto ONE
`lui at, 0x1200` base with the low word in the store offset
(`sd imm(at)`); it also hoists all three struct loads to the top and reorders
the stores. Result: 24 instructions, ~30 word differences.

The original instead materializes EACH address into its own register
(`lui r, 0x1200; ori r, r, lo`) with the value loads interleaved just before
first use, using v0/a0/a1/a2/v1/t0. The form that reproduces it byte-for-byte
is a **volatile** constant-address cast per store:

```cpp
*(volatile u64*)GS_BGCOLOR = 0;
```

Volatile suppresses the shared-base store fusion and the load hoisting. Note
the 0x12000000 and 0x120000A0 pair STILL CSEs its `lui a0, 0x1200` (the
0x12000000 store uses the lui'd register directly since its lo is 0; the
0x120000A0 store gets the later `ori a0, a0, 0xA0`) — matching the original.

Probes (decomp_state/probes/):
- v1 (plain casts, default flags): 24 instr, no match.
- v1 + -fno-schedule-insns: still shared-base form, no match.
- v3 (volatile casts, default flags): 120/120 bytes match, 0 differences.

## Register names
GS register names/offsets follow the Emotion Engine Reloaded Ghidra
extension's map (data/languages/mmio.sinc), which numbers the display-control
registers in 8-byte slots: PMODE 0x00, SMODE1 0x10, SMODE2 0x20, ...
DISPFB1 0x70, DISPLAY1 0x80, DISPFB2 0x90, DISPLAY2 0xA0, ... EXTWRITE 0xD0,
BGCOLOR 0xE0.

## Verification
- Probe candidate: 120/120 bytes at 0x1F3958, 0 differences.
- `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` byte-for-byte.
