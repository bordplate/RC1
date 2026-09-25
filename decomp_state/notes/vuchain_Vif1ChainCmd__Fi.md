# vuchain Vif1ChainCmd (func_00233F78)

Address 0x00233F78, 0x34 (52) bytes, leaf (no frame, no calls). SN TU with
`-mno-split-addresses` (vuchain.o).

## What it does

VIF1 chain command appender. Appends a 16-byte (4-word) packet to the VU1
command chain and advances the chain head by one packet:

```
packet[0] = cmd + 0x90000000
packet[1] = 0
packet[2] = 0
packet[3] = 0
*vu1ChainHead = head + 4   (4 words = 16 bytes)
```

`head` is the current chain head, the value of `*vu1ChainHead` (vu1ChainHead is
at 0x160F00, a pointer to the chain buffer). The name `Vif1ChainCmd(int)` and
the semantics were recovered from the Deadlocked debug symbols
(`reference/dl/game_dl/vuchain.cpp`), whose body is byte-identical:
`*piVar1 = cmd + -0x70000000` (== `cmd + 0x90000000`) then zero the other three
words and advance the head.

The draw pipeline calls it with VIF1 command values to intersperse VIF1
commands among the VU1 draw data. Callers and their args:
- `Transition_DefaultDraw__Fb` (0x1EB410): 0x2010000, 0x2020000, 0x2040000, 0x2080000
- `DrawDebugProfiler` (0x1F39D0): 0x2010000, 0x2020000, 0x2040000, 0x2080000
- `func_0022F288` (space.cpp, 0x22F288): 0x2010000

## Matching C

```cpp
void Vif1ChainCmd(int cmd) {
    u32* head = (u32*)vu1ChainHead;
    u32* base = head;
    head = head + 4;
    vu1ChainHead = head;
    base[0] = cmd + 0x90000000;
    base[1] = 0;
    base[2] = 0;
    base[3] = 0;
}
```

## Codegen notes (the subtle part)

The original's 13-instruction sequence:

```
lui $2, %hi(vu1ChainHead)      # load head
lw  $2, %lo(vu1ChainHead)($2)
lui $3, 0x9000                 # 0x90000000 hi
addu $4, $4, $3               # a0 (cmd) += 0x90000000
daddu $5, $2, $0              # base = head  (MOVE to a separate register)
addiu $2, $2, 0x10            # newHead = head + 0x10 (in the LOADED register)
lui $1, %hi(vu1ChainHead)
sw  $2, %lo(vu1ChainHead)($1) # advance: ABSOLUTE store (lui/sw, not GP-relative)
sw  $4, 0($5)                 # base[0] = cmd
sw  $0, 0xC($5)              # base[3] = 0
sw  $0, 0x4($5)              # base[1] = 0
jr  $31
sw  $0, 0x8($5) [delay]      # base[2] = 0
```

Three non-obvious requirements, each verified by probe:

1. **Two live head values force the move.** `base` (the original head, used for
   the 4 packet stores) and `head` (advanced to head+4, stored back) must be
   two *distinct* C values. `u32* base = head; head = head + 4;` makes them
   differ, so EGC keeps them in separate registers and emits the
   `daddu $5, $2, $0` move + `addiu $2, $2, 0x10` in the loaded register. A
   single `head` variable CSEs the base into the loaded register and computes
   newHead into a temp — dropping the move (48 bytes instead of 52).

2. **Absolute head-advance store.** Storing the advance through the `.data`
   name (`vu1ChainHead = head;`) emits the original's self-based absolute
   `lui $1; sw $2, %lo($1)`. Storing through the plain `vu1ChainHeadStore`
   alias (as VU1_addDataRef does) would emit a GP-relative store instead — the
   wrong mode here.

3. **Store order.** Writing `base[0]` (the cmd store) first, then the three
   zero stores in source order `base[1], base[2], base[3]`, reproduces the
   machine order `base[0], base[3], base[1], jr, base[2] (delay slot)` via the
   usual three-constant-store permutation.

The `cmd + 0x90000000` is a plain `lui 0x9000; addu` (the constant's lo part is
0). The param must be `int` (signed) so the cfront symbol is `Vif1ChainCmd__Fi`
(`u32` would mangle to `...__Fj`).

## Verification

- `tools/decomp_probe.py` byte-for-byte (52/52, no diffs).
- Clean `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf` passes.

Matched 2026-09-25.
