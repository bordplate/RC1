# Hud_GetIconIndex__Fi (0x1FEE38, 76 bytes) — matched 2026-09-23

## Semantics
Returns the 0-based index of `iconId` in the hud icon table. The table is an array
of 8-byte `HudIconDef` entries (`u16 id; u16 len; u16 start; u8 animType; u8 speed;`
— field names from Deadlocked `icon_t`); `id` is at offset 0 and `0xFFFF`
terminates. Element 0 is index 0; the index of the `0xFFFF` terminator is returned
if the id is not found.

## Data
- Table pointer = `hudHeap.iconTable` = 0x19A404 = `hudHeap` (0x19A3E8) + 0x1C.
  Added `u32 iconTable` (offset 0x1C, after `pad_18`) to the `HudHeap` struct.
- Callers: `GetIconFrame__Fii` (0x1FF960) and `func_001FF500` both load
  `mem[hudHeap+0x1C]` and index with `index << 3` (index*8), confirming the
  8-byte entry stride.

## Codegen (the hard part)
The original has a **peeled prologue** (element 0 checked separately) with a
specific register layout that EGC 2.95.2 does not produce naturally. Three tricks:

1. **Inverted condition.** Write the body as
   `if (value != end && value != iconId) { ...loop... }` (a *positive* condition
   to ENTER the loop) rather than `if (value == end) return; if (value == id)
   return;`. The positive form makes EGC emit `beq` (return-as-jump) for the two
   peeled element-0 checks — matching the original. The early-return form makes
   EGC invert them to `bne` (continue-as-jump), which does not match.

2. **Register pins.** Pin the prologue locals to `$5` (table), `$6` (index/count),
   `$2` (value), `$3` (0xFFFF) and the loop locals to `$3` (table) and `$5`
   (0xFFFF). This forces the `a1<->v1` swap: in the prologue the table ptr is in
   `a1($5)` and 0xFFFF in `v1($3)`; in the loop the table ptr moves to `v1($3)`
   and 0xFFFF to `a1($5)`. Without the pins EGC keeps the table ptr in one
   register throughout and allocates the loop value to the wrong register.

3. **SN assembly.** The loop's two filler `nop`s (between `count++` and the `bne`)
   are inserted by **SN ps2eeas**, not by cc1 and not by GNU as. A GNU-assembled
   build of the same C is 68 bytes (nops missing). `hud.o` is one of the five GNU
   compatibility TUs, so `Hud_GetIconIndex` was split into its own SN-assembled TU.

## Splat boundary split
`hud.cpp` is a GNU TU (Makefile lines 96-101). To give this one function SN
assembly, add a Splat subsegment boundary in `config/RC1.yaml` at the next-function
file offset:
```yaml
- [0xffdb8, cpp, game/hud_icon]   # Hud_GetIconIndex (SN, default)
- [0xffe08, cpp, game/hud]        # rest of hud.cpp (GNU override)
```
`hud_icon.o` uses the default `-snas`; the rest of `hud.o` keeps the GNU override.
The linker script places `hud_icon.o(.text)` immediately before `hud.o(.text)`,
preserving the intra-TU address order (Hud_GetIconIndex is the first function).

## Verification
- `tools/decomp_probe.py` on the isolated candidate: 76/76 bytes, 0 differences.
- Full build: `cmp build/boot_elf.elf assets/boot_elf.elf` passes (byte-for-byte).

## Note on the struct
`HudHeap` (grown with `iconTable`) is defined in both `hud.cpp` (for
`Hud_HeapReset`'s named `heapCursor`/`heapEnd` fields) and `hud_icon.cpp` (for the
function). The duplicate typedef is intentional (no header in this project for this
struct); the struct growth does not change `Hud_HeapReset`'s codegen (it only uses
the 0x10/0x14 fields).
