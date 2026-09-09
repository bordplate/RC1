# mobyutil func_002141F8 (0x2141F8, 0x30 bytes)

Matched 2026-09-09. Returns the first pointer of the table pointed to by
`MobyInstance` field at 0x78 when the 0x20 mode bit (field at 0x34) is set,
else 0:

```c
extern "C" void* func_002141F8(struct MobyInstance *m) {
    if (!m)
        return 0;
    int t = *(u16*)((char*)m + 0x34) & 0x20;
    asm volatile("nop\n\t" "nop\n\t" "nop");
    if (t) {
        void* p = *(void**)((char*)m + 0x78);
        return *(void**)p;
    }
    return 0;
}
```

Twin `func_00214228` is byte-identical except the final `lw v0,0(v1)` is
`lw v0,16(v1)` (table word at +0x10 instead of +0x00).

## Raw offsets instead of struct fields

The project's `MobyInstance` (code/include/mobyfunc.h) is 8 bytes SHORT of the
original layout by the time we reach these fields. Measured with the project
EGC (`-G8 -O2`): `modeBits` at 0x2C and `pVar` at 0x68, while the original
binary loads `lhu v0,0x34(a0)` and `lw v1,0x78(a0)`.

Root cause (strong evidence, full re-derivation pending): `s128` in
code/include/types.h is `long long` (8 bytes), so
`struct vec4 { s128 ...; }` is 8 bytes. The project's `MobyInstance.pos` at
0x10 therefore occupies only 8 bytes, but the original needs it to occupy 16:
both affected fields shift by exactly +8 (modeBits 0x2C->0x34, pVar 0x68->0x78),
which is the signature of a single 8-byte-too-small field (`pos`) before them.
A 16-byte `pos` reproduces modeBits=0x34 exactly; pVar lands 4 bytes high in a
rough manual model, so the intermediate field ladder still needs a careful
re-derivation against generated asm before the header is rewritten.

So `vec4` is very likely modelled 8 bytes too small project-wide. Fixing
`s128`/`vec4` (and re-deriving MobyInstance) is a project-wide change that
affects every vec4 user and must be re-validated against all 157 matched
functions; it was deliberately NOT done as part of this one-function commit.
Until that lands, functions that need MobyInstance fields at/after `pos` must
use raw byte offsets (verified against the original binary) like above.

## The three-nop trick

Original body:

```
andi v0,v0,0x20
nop
nop
beqz v0, exit      ; delay slot: nop
lw   v1,0x78(a0)
jr   ra
lw   v0,0(v1)
```

Natural C (no asm) compiles to 9 words: the compiler puts the `lw` in the
beqz delay slot and emits no nops. The EE assembler (ee/bin/as, invoked by
ee-gcc -c) will move the `lw` OUT of a conditional-branch delay slot and
replace the dslot with a nop — but it consumes one pre-existing explicit nop
for that. So to end up with TWO nops between andi/beqz plus the dslot nop,
the compiler .s must contain THREE nops after the andi. Emitting them via one
`asm volatile("nop\n\t" "nop\n\t" "nop");` statement (placed after computing
`int t = ... & 0x20;` and before `if (t)`) gives the exact 12-word original.

Two asm nops gives one too few (assembler eats one); verified by object
diff. Note the earlier C1_asm2 probe (two nops) produced 11 words — do not
copy that form.

## Verification

- Compiled object .text = the original 12 words exactly:
  `54800003 94820034 03e00008 0000102d 30420020 00000000 00000000 1040fffa
  00000000 8c830078 03e00008 8c620000` (verified with a `make probe` object
  before the probe files were cleaned up).
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf` passes; the built
  ELF disassembly of 0x2141F8..0x214227 is byte-identical to the original.
