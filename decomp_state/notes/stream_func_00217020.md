# func_00217020 (code/game/stream.cpp) — matched 2026-09-04

## Semantics

Zeros two fields of the struct that lives at .data vram 0x0013C940
(symbol `D_0013C940`, defined in the Splat linker script as an absolute
assignment):

- word store  to base+0x190 = 0x0013CAD0
- half store to base+0x18E = 0x0013CACE (low byte of the word at +0x190)

Sole caller: vram 0x001EA7C4, `jal func_00217020` inside the large
still-assembly function starting at 0x001E9B10 (bloaders/loader region).
The same struct's neighbours are used by stream.cpp funcs:
DAT_0013CAD4 (+0x194, result of FUN_00124a88), DAT_0013CAD8 (+0x198) and
DAT_0013CADC (+0x19C) are zeroed by the sibling at 0x00217048, so this is
a "clear stream state header" step of a level/resource load.

Ghidra decompiler named them DAT_0013cad0/DAT_0013cace; only this function
references them (xref-checked), so declaring the base struct locally in
stream.cpp is safe.

## Original body (file offset 0x117FA0, vram 0x00217020, 5 words)

```
3c020014  lui   $v0, 0x14            # hi(0x13C940), lo adjusted
2442c940  addiu $v0, $v0, -0x36c0    # base = D_0013C940
900140ac  sw    $zero, 0x190($v0)
0800e003  jr    $ra
a440018e    sh   $zero, 0x18e($v0)   # delay slot
```

## Replacement (stream.cpp)

```cpp
typedef struct {
    u8 pad[0x18E];
    u16 field_18e;
    u32 field_190;
} StreamState;

extern StreamState D_0013C940 __attribute__((section(".data")));

extern "C" void func_00217020(void) {
    D_0013C940.field_18e = 0;
    D_0013C940.field_190 = 0;
}
```

## Codegen findings (EGC 2.95.2, project flags)

1. Plain pointer math `*(u32*)(D_0013C940+0x190)=0; *(u16*)(D_0013C940+0x18E)=0;`
   with a `u8[]` extern does NOT match: EGC materializes the second address as
   its own register (`addiu v1,v0,0x190` then `sh zero,-2(v1)`) — an extra
   instruction. A struct whose fields sit exactly at the target offsets makes
   both stores direct `base+imm` and matches (one shared lui/addiu pair).

2. Store order for two independent CONSTANT stores sharing one %hi/%lo base:
   machine order came out as the REVERSE of C statement order in both
   attempts, i.e. the FIRST statement's store lands in the `jr $ra` delay
   slot and the SECOND statement's store precedes the jr:

   - source [sw+0x190, sh+0x18E] -> machine [sh+0x18E main, sw+0x190 delay]
   - source [sh+0x18E, sw+0x190] -> machine [sw+0x190 main, sh+0x18E delay]

   This is the opposite of the strfile_func_0023BA48 observation (stores of
   distinct argument registers emitted in source order). Likely driver: here
   both store data are the $zero constant and the base register is a computed
   temporary, so the optimizer's insn ordering differs. When matching such a
   tail, try BOTH statement orders; only the object/ELF diff decides.

## Verification (mechanical)

- `make build/code/game/stream.o`, objdump of func_00217020:
  `lui v0 / addiu v0,v0 / sw zero,400(v0) / jr ra / sh zero,398(v0)` with
  R_MIPS_HI16/LO16 relocs — identical instruction sequence to the original.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- ELF slice at file offset 0x117FA0 (20 bytes) built == original:
  `1400023c 40c94224 900140ac 0800e003 8e0140a4`.
- No symbol renamed, so no stale-include hazard; the caller (still
  INCLUDE_ASM) references func_00217020 by name and links unchanged.

## Follow-ups

- The struct at D_0013C940 deserves a real name once 0x001E9B10 (its main
  user) and the stream.cpp neighbours are decompiled; fields around +0x18E..
  +0x19C form the state header. Renaming func_00217020 then requires updating
  the jal reference in the caller's generated .s (touch its source first).
