# ResetGsRegisters__Fv (0x1F3868, 0xEC) — MATCHED 2026-09-28

`code/game/draw_post_post.cpp`. Appends two 16-byte VIF data-reference
packets to the VU1 command chain, then loads the packed fog color into a GS
register:

- Packet A `[0x30000013, &resetGsRegsFixed, 0, 0x50000013]` at the head.
- `vu1ChainHead = head + 4` via an ABSOLUTE store (mid-function).
- Packet B `[0x3000000B, &vu1GsRegsFont, 0, 0x5000000B]`; its first word is
  written through a LOCAL pointer (`next = head + 4; next[0] = tag`) so the
  store reuses the old-head register + 0x10 with no reload.
- `vu1ChainHeadStore = head + 4` via a GPREL store in the `jal` delay slot.
- `VU1_addGSregister(0x3D, fogR | fogG<<8 | fogB<<16)`.

New symbol: `resetGsRegsFixed = 0x0013CFC0` (config/symbols.txt) — a GS
register reset block (record header 0x00008012); the next record
(0x00008001) starts at its end. `VU1_addGSregister__FUiUlb` (0x233980)
exports (unsigned int, unsigned long, bool) but ignores the bool; no RC1
call site materializes it, so the source declares a 2-param prototype with
`asm("VU1_addGSregister__FUiUlb")` (EGC 2.95.2 accepts the free-function asm
label; R_MIPS_26 emitted).

## Solution

```cpp
extern volatile u32* volatile vu1ChainHead;      // PLAIN (no .data)
extern volatile u32* vu1ChainHeadStore;          // linker alias, 0x160F00
extern u32 vu1GsRegsFont[];
extern u32 resetGsRegsFixed[];
void VU1_addGSregister(unsigned int reg, unsigned long value)
    asm("VU1_addGSregister__FUiUlb");

void ResetGsRegisters() {
    vu1ChainHead[0] = VU1_DATA_REF_TAG | 0x13;
    vu1ChainHead[1] = (u32)resetGsRegsFixed;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 0x13;
    volatile u32* next = vu1ChainHead + 4;
    vu1ChainHead = next;
    next[0] = VU1_DATA_REF_TAG | 0x0B;
    vu1ChainHead[1] = (u32)vu1GsRegsFont;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 0x0B;
    vu1ChainHeadStore = vu1ChainHead + 4;
    VU1_addGSregister(VU1_FOG_COLOR_GS_REG,
        (long)viewCtx.fogR | ((long)viewCtx.fogG << 8) | ((long)viewCtx.fogB << 0x10));
}
```

No private flags: `draw_post_post.o` keeps project defaults. No pins, no
barriers.

## Key finding: PLAIN `-G8` head declaration beats the vuchain `.data` form

The vuchain pattern (double-volatile `.data` pointer + `-mno-split-addresses`)
does NOT match here. Under `-mno-split-addresses` the streamed block
addresses (`resetGsRegsFixed`, `vu1GsRegsFont`) and the fog base become
single indivisible `la` pseudos in the compiler `.s`, so the scheduler cannot
interleave the `[0]A` store between the D `lui`/`addiu` halves, and the fog
base folds to `la $9,viewCtx+560` (offsets 0/4/8) instead of the original's
raw `viewCtx` base (offsets 0x230/0x234/0x238). That left 25-30 residual
word diffs (C2/C3/F constant regs, L3/L4 head-load phase, Lu register,
[0]A interleave, fog base) that no source reordering, register pin
(head0 `$4`, next `$3`), or scheduler flag (`-fno-schedule-insns[2]` made it
worse) could close.

The matching form is the PLAIN (non-`.data`) double-volatile declaration with
DEFAULT address splitting: at project-default `-G8` EGC classifies the 4-byte
pointer as small-data and emits a bare `lw r,vu1ChainHead` pseudo that
ps2eeas expands in place to the self-based `lui r; lw r` load, while
`resetGsRegsFixed`/`vu1GsRegsFont`/`viewCtx` stay independently schedulable
`lui/%hi` + `addiu/%lo` pairs (see the 2026-09-16 -G8 bare-pseudo
observation). The mid `sw r,vu1ChainHead` stays absolute; the final
`sw r,vu1ChainHeadStore` in the `jal` noreorder delay slot is GPREL16. This
reproduces the whole original register map (L1 a0, D v1, C2 a1, C3 t1, F
a2, C4 t0, head loads alternating v0/v1, fog via a3 raw base) on the first
probe (0 word diffs of 59).

The Deadlocked descendant (reference/dl/game_dl/draw.cpp:1931) confirms the
local-pointer structure (`paiVar1 = vu1_bufPtr + 1; vu1_bufPtr = ...; 
(*paiVar1)[0] = tag;`).

Fog packing: the `(long)` casts on each field give the original's 64-bit
`dsll 8` / `dsll 16` shifts with 32-bit `or` combines and G/B/R load order;
the value is passed as the callee's `unsigned long` second arg.
