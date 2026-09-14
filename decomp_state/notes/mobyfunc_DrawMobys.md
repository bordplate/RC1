# DrawMobys (0x20D460, 0x80) — MATCHED 2026-09-14

Per-frame moby draw pass. It sets up the frame, and — if a global gate is set —
reinitializes the moby class distance tables, advances the moby animation chain
through the handwritten `MobyProc`, and reports a VU1-chain overflow if the chain
head has passed its limit.

## Behavior

```
DrawMobysSetup();
if (D_0018A2D8[0] != 0) {
    InitMobyClassDists();
    ret = MobyProc(*(int*)0x15FF18, *(int*)0x15FF14, -1, 1);
    D_0015FF14 = ret;                          // UNCONDITIONAL within the gate
    if (*(int*)0x160F00 > *(int*)0x160F08)
        STUB_printf(vuChainOverflowMessage);   // 0x1E8400
}
DrawMobysCleanUp();
```

- `D_0018A2D8` (0x18A2D8) is the per-frame gate. It is read only here in the
  boot ELF (the writer is level code that is not resident), and its initial
  `.data` value is 0. Retained as an address name; declared as an array so the
  load is an absolute `lui v0; lw v1` pair as in the original (a scalar extern
  would be GPREL16, which is invalid out of the gp window and also not the
  original's encoding).
- `MobyProc` (0x211808) is Insomniac's handwritten assembly (see
  `config/RC1.yaml`: `[0x10e7e8, asm, game/mobyproc]`), referenced only by the
  generated assembly in this file; the linker pins it. It is a 4-arg `int`
  routine: `(head=*(0x15FF18), chain=*(0x15FF14), -1, 1)`.
- `D_0015FF14` is the moby animation chain head. `DrawMobysSetup` copies the
  `D_0015F638` chain head into it, and `MobyProc`'s return value is written back.
  The write is NOT guarded by the overflow test: in the original the
  `sw v0,-27884(gp)` sits in the *delay slot* of the `beqz` that skips the
  `printf`, so it executes on both branch paths.
- `0x160F00` (`vu1ChainHead`, the VU1 bump pointer) is compared against the
  limit at `0x160F08`; on overflow a message is printed (see
  notes/vuchain_VU1_addDataRef__FPvi.md for the chain-head family).

## Codegen (default flags, mobyfunc.o)

Four in-window VALUE loads must be constant-address casts. Named references
(scalar → GPREL16, array → two-register base-$v0) do not reproduce the
original's self-based `lui r; lw r,off(r)` pairs. Expressed as an `enum`
constant per the ProcessMobyAnimData precedent (see
notes/mobyfunc_ProcessMobyAnimData__Fv.md); the addresses are the linker
symbols `D_0015FF14` / `D_0015FF18` (build/data/lit.lit4.s), `vu1ChainHead`
(0x160F00) and its limit slot 0x160F08.

The one store (`D_0015FF14 = ret`) is the opposite case: it needs a plain
scalar `extern int` so EGC emits the GPREL16 `sw v0,-27884(gp)` the original
uses in the `beqz` delay slot (a constant-cast store would be absolute
`lui at; sw`).

The head-vs-limit test is written `head > limit` (head first) so EGC's `slt`
operand order (`slt v1,v1,a0` with head in $a0) matches; `limit < head` emits
the operands swapped.

## Symbols added

- `vuChainOverflowMessage = 0x001E8400;` in `config/symbols.txt` (the
  `"vu chain overflow - mobys dropped\n"` string).

All 32 words match; `mobyfunc.o` matches and `cmp build/boot_elf.elf
assets/boot_elf.elf` passes.
