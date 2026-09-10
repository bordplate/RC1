# mobyutil func_00214228 (0x214228, 0x30 bytes)

Matched 2026-09-10 as `moby_getSecondaryObject`. Byte-identical twin of the
matched `moby_getActiveObject` (func_002141F8): same gate (0x20 bit of the u16
at MobyInstance+0x34) and same first pointer load (`lw v1,0x78(a0)`), but the
final return load is `lw v0,0x10(v1)` instead of `lw v0,0(v1)` — it returns the
second word of the per-moby variable block instead of the first.

```c
void* moby_getSecondaryObject(struct MobyInstance* m) {
    if (!m)
        return 0;
    int t = *(u16*)((char*)m + 0x34) & 0x20;
    asm volatile("nop\n\t" "nop\n\t" "nop");
    if (t) {
        void* p = *(void**)((char*)m + 0x78);
        return *(void**)((char*)p + 0x10);
    }
    return 0;
}
```

## Evidence

- Ghidra FUN_00214228: `if ((param_1 != 0) && ((*(ushort*)((int)param_1+0x34) & 0x20) != 0)) return *(undefined4*)(*(int*)((int)param_1+0x78)+0x10); return 0;`
- Both callers are in func_00213920 (mobyutil, still INCLUDE_ASM, 8-arg
  function): jal at vram 0x213E44 and 0x213E60, passing $s7 (a
  MobyInstance*; confirmed by `lwc1 $f0,16(s7)`/`lwc1 $f1,20(s7)` reading the
  0x10 `pos` floats). First result: `if (r) ((u8*)r)[0x2E] = 1;` Second result:
  `if (!r) ((u8*)r)[0x2E] = 0;` (the null path stores through address 0x2E —
  an original quirk, presumably a never-taken path).
- The sibling call at 0x213A10 targets the twin (0x2141F8) with the same
  $s7; its result is used as an object with a u8 state at +8, an s16 timer at
  +0x16 (set to -1), and a countdown at +0x1C (decremented via 0x1F9770).
  Both slots therefore point at the same kind of object; the +0x00 slot is
  the "active" one, the +0x10 slot the secondary one — hence the name.
  Naming is evidence-based but the object type itself remains unidentified.
- The per-moby variable block is the 0x80-byte slice of the MobyVars bss
  assigned in CreateMoby (mobyfunc.cpp) and zeroed with FastMemSet(...,0x80);
  MobyInstance+0x78 (`pVar`) points at it.

## Mechanism

Copied the twin's verified recipe (see notes/mobyutil_func_002141F8.md):
raw byte offsets (project vec4/s128 is 8 bytes short, so struct fields for
0x34/0x78 would land at 0x2C/0x68) and one `asm volatile` with THREE nops to
survive the EE assembler eating one nop for the beqz delay slot. First probe:
12/12 words.

## Verification

- Object slot bytes (build/code/game/mobyutil.o .text+0x1600, 0x30 bytes) are
  byte-identical to the original ELF file offset 0x1151A8:
  `03008054 34008294 0800e003 2d100000 20004230 00000000 00000000 faff4010
   00000000 7800838c 0800e003 1000628c` (Splat .s comments print these
  reversed, e.g. `2D100000` for 0x0000102D).
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf` passes; count
  729 -> 728.
- Symbol handling: the C definition emits `moby_getSecondaryObject__FP12MobyInstance`;
  config/linker_aliases.ld adds `func_00214228 = 0x00214228;` so the
  still-generated caller assembly links (same pattern as the twin, see
  commit f4cb758).
