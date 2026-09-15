# DrawMobys (0x20D460, 0x80) — MATCHED 2026-09-14

Ordinary symbolic scalar C++ matches all 128 bytes with the production
2.73a compiler and SN assembler (`-snas`). The current game source retains
its older numeric-address implementation, which also passes full-image parity.
See `symbolic_address_pipeline.md` for the symbolic probe and relocation tests.

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

## Codegen

SN expands ordinary scalar extern loads into the original self-based
`lui r; lw r,off(r)` pairs for `D_0015FF14`, `D_0015FF18`, `vu1ChainHead`
(0x160F00), and its limit slot 0x160F08. GNU as instead relaxes these extern
loads to GP-relative instructions; numeric-address loads were used to work
around that behavior in the existing source. They are not required by SN.

The same plain scalar extern also gives the original GPREL16 store
`sw v0,-27884(gp)` in the `beqz` delay slot. SN handles both access forms
without aliases or separate numeric load expressions.

The head-vs-limit test is written `head > limit` (head first) so EGC's `slt`
operand order (`slt v1,v1,a0` with head in $a0) matches; `limit < head` emits
the operands swapped.

## Symbols added

- `vuChainOverflowMessage = 0x001E8400;` in `config/symbols.txt` (the
  `"vu chain overflow - mobys dropped\n"` string).

All 32 words match; `mobyfunc.o` matches and `cmp build/boot_elf.elf
assets/boot_elf.elf` passes.
