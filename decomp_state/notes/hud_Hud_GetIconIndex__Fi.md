# Hud_GetIconIndex__Fi (0x001FEE38, 76 bytes) — BLOCKED

Status: blocked (2026-09-13). Last-resort-decompiler invoked; all recommendations tested.

## Function
Linear search of an icon table. `hudIconList` (global pointer at 0x19A404, .data)
points at an array of 8-byte `HudIconDef` entries; `entry->code` is a u16 at
offset 0; the list is 0xFFFF-terminated. Returns the index k of the first entry
with `code == arg`, or k of the 0xFFFF terminator if absent. Entry 0
match/terminator returns 0. Both `count++` and `icon++` execute on the
terminating iteration (they sit in branch delay slots), so the return value on a
terminator is the terminator's index.

Entry layout (8 bytes): +0 u16 code, +2 u16 frameCount, +4 u16 frameBase,
+6 u8, +7 u8. (frameCount/frameBase confirmed against callers
`GetIconFrame__Fii` / `FUN_001ff500`.)

## Original ground truth (objdump of assets/boot_elf.elf)
```
1fee38: lui   v0,0x1a
1fee3c: lw    a1,-23548(v0)      # a1 = *0x19A404 (table)
1fee40: li    v1,0xffff
1fee44: lhu   v0,0(a1)           # entry0.code
1fee48: beq   v0,v1,0x1fee7c     # exit if entry0==0xffff
1fee4c: move  a2,zero            # count=0 (delay)
1fee50: beq   v0,a0,0x1fee7c     # exit if entry0==arg
1fee54: move  v1,a1              # v1 = table (delay)
1fee58: li    a1,0xffff          # a1 repurposed as 0xffff
1fee5c: addiu v1,v1,8            # v1 = entry1
1fee60: lhu   v0,0(v1)           # loop
1fee64: beq   v0,a1,0x1fee7c     # exit if 0xffff
1fee68: addiu a2,a2,1            # count++ (delay, ALWAYS)
1fee6c: nop
1fee70: nop
1fee74: bne   v0,a0,0x1fee60     # continue if != arg
1fee78: addiu v1,v1,8            # icon++ (delay, ALWAYS)
1fee7c: jr    ra
1fee80: move  v0,a2              # return count
```

## Closest candidate (register-pinned, last-resort Test 1 form)
```cpp
typedef struct { u16 code; u16 frameCount; u16 frameBase; u8 unk_06; u8 unk_07; } HudIconDef;
extern HudIconDef *hudIconList __attribute__((section(".data")));
int Hud_GetIconIndex(int code)
{
    register HudIconDef *first asm("$5") = hudIconList;   // a1
    register int index asm("$6") = 0;                     // a2
    register int value asm("$2") = first->code;           // v0
    register int firstEnd asm("$3") = 0xffff;             // v1
    if (value != firstEnd && value != code) {
        register HudIconDef *icon asm("$3") = first;      // v1
        register int end asm("$5") = 0xffff;              // a1
        ++icon;
        for (;;) {
            value = icon->code;
            ++index;
            if (value == end)
                break;
            if (value != code) {
                ++icon;
                continue;
            }
            break;
        }
    }
    return index;
}
```
This reproduces EVERY real instruction and the exact register allocation
(table=a1, pre-check 0xffff=v1, count=a2, value=v0, loop pointer=v1,
loop 0xffff=a1) — including the pre-check (two `beq` to a single shared exit,
`count=0` in the first delay slot) and the loop (`lhu; beq(A)->exit [count++
delay]; bne(B)->loop [icon++ delay]; fall-through -> exit`). The hard-register
`asm("$reg")` pins are what stop the local cc1 from peeling/rotating the loop
(unpinned forms all peel: first `count++` hoisted to `li reg,1` + `b`, two
separate exits, 84-100 bytes).

## The ONLY difference: two nops
Candidate is 68 bytes vs 76; it matches word-for-word EXCEPT it omits the two
nops at 0x1FEE6C and 0x1FEE70. The local cc1 packs the `bne` immediately after
the `beq`'s delay slot (beq at X, count++ at X+4, bne at X+8); the original
schedules the `bne` two slots later (beq at X, count++ at X+4, nop, nop, bne at
X+16). Every non-nop word is identical, including branch targets (once the 8-byte
size difference is accounted for) and the `jr ra; move v0,a2` epilogue.

This is a delay-slot/block scheduling difference between the local EGC
(`gcc version 2.9-ee-991111b/r4`) and Insomniac's original build: the original
leaves two empty slots between the two in-block branches; the local fills them by
hoisting the `bne` up. There is no source-level dependency that would force the
gap (the `bne` operands v0/a0 are independent of the intervening slots), so no C
form can make the local scheduler reserve those two slots.

## Tried (all via tools/decomp_probe.py, standalone, project flags)
- 17 C forms: pre-check + for(;;)/while(1)/for-clause/do-while; while cond-at-top
  (no pre-check); if/else; explicit-else break; `if (value==code) break; ++icon;
  continue;` (t1d, diverges); hoisted-increment variants; redundant pre-check;
  volatile `HudIconDef*` (V1/V2, adds `andi v0,v0,0xffff`, still peels).
- Last-resort Test 1 (register pins) = the 68-byte/2-nop candidate above.
- Last-resort Test 2 (add `asm volatile("" : "+r"(icon), "+r"(index))` barrier at
  loop top): identical 68-byte/2-nop result.
- Flags on Test 1: -O1 (100B, worse), -fno-schedule-insns, -fno-schedule-insns2,
  both, -fno-strength-reduce, -fno-thread-jumps, combos, -G0 — none change the
  two-nop layout (peeling/scheduling unaffected; cc1plus has no -fpeel flag).

## Why not a match
Byte-for-byte parity is required; the two missing nops (8 bytes) break it. The
local EGC's instruction scheduler cannot be coaxed (by any tested C form,
declaration, attribute, or flag) into leaving the two slots between the `beq`
delay and the `bne` empty. The only matched backward-branch loop in the tree
(func_001F0B88) is a counted store-loop, a different pattern with no prior art for
this two-branch search shape.

## Recommendation for a future pass
Revisit if a different EGC build/flag set is introduced, or if a scheduler flag
that controls intra-block branch packing is discovered. The semantic C is settled
(Test 1 form); only the two-nop scheduling is the gap.
