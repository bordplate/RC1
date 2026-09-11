# func_001EAF50 -> Help_LoadMsgs (matched 2026-09-11; re-matched with named pointers 2026-09-11)

## Semantics
Loads a help message set for the transition:

- `helpMsgData` (0x15EF60, in gp window) — a global holding the help-message data
  base pointer.
- `helpOffsetTable` (0x15EF64, in gp window) — a global holding the per-set offset
  table pointer. `Transition_LoadWad` (0x1EA830) does `*helpOffsetTable =
  *helpMsgData` before calling `Help_LoadMsgs(i)` (i=0..7, plus one `Help_LoadMsgs(0)`).
- `node = (char*)*helpMsgData + ((int*)*helpOffsetTable)[set]` (raw 32-bit addu, no
  scale).
- `HelpMsgCount` (0x1996FC, out of gp window) = `*node` (message count for the set).
- `HelpMsgs` (0x15F6A0, in gp window) = `(struct HelpMsg*)(node + 8)` (entry array).

void return; all three call sites ignore v0. `Help_FindIndex` (0x1FDCA0) reads the
count at 0x1996D0+0x2C = 0x1996FC. The helpMsgData/helpOffsetTable globals are only
read in the boot ELF (overlay code sets them).

## Renames
- `func_001EAF50` -> `Help_LoadMsgs__Fi` (config/symbols.txt).
- `Help` (0x0196FC) -> `HelpMsgCount` (config/symbols.txt).
- 0x15EF60 -> `helpMsgData`, 0x15EF64 -> `helpOffsetTable` (config/symbols.txt); both
  are read here and written by `Transition_LoadWad`.

## Match details
Original layout (14 insns):
`t pair -> a1 (fused); sll a0; b pair -> v1 (fused); lui a2 (hoisted, before the
table consume); addu; elem lw -> v0; addu v1 in-place; count lw -> a0; addiu v1,8;
sw count main; jr ra; sw msgs(gp) delay`.

The pointer loads are absolute single-register pairs: the original uses
`lui $a1,0x16; lw $a1,-0x109C($a1)` for the table and `lui $v1,0x16;
lw $v1,-0x1098($v1)` for the base. A plain gp-window extern compiles to GP-relative
loads (wrong mode), and with address splitting ENABLED a named symbol emits a
two-register load (`lui $Y; lw $Z`, Y != Z) that the allocator regroups — neither
matches. (~95 natural-C forms and 5 flag combos were tested in the first pass before
last-resort escalation; none of them got the in-window pointers to single-register
absolute loads while keeping the count store correct.)

Key finding (expert subagent): with `-mno-split-addresses` a named-symbol load
becomes a single pseudo (`lw $reg, sym`) that the assembler expands to a self-based
`lui $reg; lw $reg` — exactly the original's single-register absolute pattern. This
is the same lever that reproduced VU1_addDataRef's self-based lui/lw sequences.
Base-first source order (`base = helpMsgData;` then `offset = helpOffsetTable[set];`)
is required to land the loads in a1/v1 in the original order. The `.data` section
attribute forces absolute (lui/lw) loads for the in-window pointer globals.

The count store still needs a pin: the original uses a hoisted `lui $a2,0x1A0000`
then `sw $a0,-0x6904($a2)`. 0x1A0000 is HelpMsgCount's (0x1996FC) high-16 page — the
ELF is stripped, so there is no real symbol to name it; it stays a documented
constant. The low-16 offset comes from the named symbol via `%%lo(HelpMsgCount)`.
EGC reproduces the hoisted-into-a2 + base-a2 split only with the page pinned to $6
(+ a zero-byte `"+r"` barrier) and the count pinned to $4. Computing `node + 8`
BEFORE the store is required (the original orders `addiu v1,8` before the count
store; the reverse order swaps the last two words).

Final matched source (transition.cpp):

```cpp
extern char* helpMsgData     __attribute__((section(".data")));
extern int*  helpOffsetTable __attribute__((section(".data")));
extern int   HelpMsgCount    __attribute__((section(".data")));

void Help_LoadMsgs(int set) {
    int base = (int)helpMsgData;
    int offset = helpOffsetTable[set];
    char* node = (char*)base + offset;
    register int helpCountPage asm("$6") = 0x1A0000;
    asm volatile("" : "+r"(helpCountPage));
    register int count asm("$4") = *(int*)node;
    char* next = node + 8;
    asm volatile("sw %0,%%lo(HelpMsgCount)(%1)" : : "r"(count), "r"(helpCountPage));
    HelpMsgs = (struct HelpMsg*)next;
}
```

## Flag
`game/transition.o` receives `PRIVATE_COMPILE_FLAGS = -fno-schedule-insns
-mno-split-addresses` (Makefile, split out from the menu group). The TU contained no
previously matched C functions, so no existing match is affected.

## Verification
Standalone probe: 56/56 bytes, 0 differences (Help_LoadMsgs__Fi). Clean
`make clean && make split && make -j2` plus `cmp build/boot_elf.elf
assets/boot_elf.elf` passes (byte-identical). transition.o references
helpMsgData/helpOffsetTable/HelpMsgCount/HelpMsgs via R_MIPS_HI16/LO16 relocations.
Count: 720 nonmatching.
