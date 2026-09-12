# pause_setFirstSpriteTag (func_0021DDD0, 0x0021DDD0, 40 bytes) — matched 2026-09-12

`code/game/pause_post.cpp:151`. Third callback of the shared
`func_0021ABF8` / `func_0021B1C8` / `func_0021DDD0` triple that appears in all
four static pause-menu item definitions (0x1CE588 + k*0x50, count 4 at
0x1CE778). The triple is (select-action handler, drawer, this tag setter).

## Semantics

```c
int pause_setFirstSpriteTag(PauseSpriteListMode* mode) {
    *(u16*)(mode->spriteList + 2) = (pauseSpriteTagByte == 1) ? 0 : 3;
    return 0;
}
```

- `mode->spriteList` (offset 0x34) is set by `pause_selectSpriteList`
  (0x21CA98) to `pauseSpriteListA` (0x1D06D0) or `pauseSpriteListB`
  (0x1D0708). Each list region is 0x38 bytes of packed u32 sprite ids (13
  ids + one zero terminator in ROM); the consumers walk it with a 0xC
  stride, treating each visited record as `{u16 spriteId @ +0x00, u16
  actionTag @ +0x02, ...}`.
- The u16 at +0x02 is the per-entry action tag consumed by the drawer
  (0 == dimmed color 0x80303030; 2 == substitute sprite 0x4F54; 3..11 ==
  select-action cases in the 0x21ABF8 switch, e.g. case 3 sets the
  next-sprite state from the entry's +0x04 field). The tag is the high half
  of the first list word: zero in ROM, written at runtime by this callback.
- `pauseSpriteTagByte` (0x1413F4, core.data, boots zero) is read ONLY by this
  function in the boot ELF (Ghidra xref + full-immediate scan). The nearest
  write found is `sb zero, 0x13F5(v1)` at 0x22FB64 (delay slot of a level
  transition handler), i.e. the byte at +1; no direct store to 0x1413F4 exists
  in any boot-ELF text section (all lui-0x14 store/load immediates scanned).
  Its writer is a computed-address store or overlay code.
- `func_0021DDD0` is referenced only by DATA (the three callback triples at
  0x1CE5C0/0x1CE610/0x1CE660); no jal/j to it anywhere. Alias
  `func_0021DDD0 = 0x0021DDD0;` added to `config/linker_aliases.ld` for
  consistency with the matched siblings (e.g. func_0021CA98).

## Codegen

Original allocation: `lui v0,hi; lw a1,34(a0); lbu a0,lo(v0); li v1,1;
beq a0,v1,L; <move v0,zero>; li v0,3; L: sh v0,2(a1); jr ra; <move v0,zero>`.

EGC 2.95.2 (project flags) reproduces it ONLY when the byte is a NAMED
out-of-window symbol:

```c
extern u8 pauseSpriteTagByte __attribute__((section(".data")));
// config/linker_aliases.ld: pauseSpriteTagByte = 0x001413F4;
```

The constant-cast form `*(u8*)0x1413F4` compiles to the same RTL semantics
but the scheduler hoists the lbu pair FIRST and the allocator assigns
`lbu v1; li v0,1; lw a0,34(a0); beq v1,v0; move a1,zero; li a1,3; sh a1,2(a0)`
— 8 of the 10 words differ (probe v1/v2 in
`decomp_state/probes/pause_setFirstSpriteTag_v1.cpp` / `_v2.cpp`); the named
symbol form (probe v3, `_v3.cpp`) is a 40/40 byte match. So: when the original
shows `lui X; lw ptr; lbu a0` with the byte in the argument register and the
hi part in v0, try a named `.data` symbol before blaming the allocator.

Verified: `tools/decomp_probe.py` match=true (40/40), full `make` +
`cmp build/boot_elf.elf assets/boot_elf.elf` passes (count 718 -> 717).
