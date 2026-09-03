# 4-byte "functions" that are dead gaps, not decompilable code

Splat emitted nonmatching INCLUDE_ASM placeholders for these 4-byte stubs:

| symbol         | vram      | bytes                     | file offset |
|----------------|-----------|---------------------------|-------------|
| func_0023ACB0  | 0x23ACB0  | `addiu $sp,$sp,0x10; nop` | 0x13BC30    |
| func_0023CBC8  | 0x23CBC8  | `addiu $sp,$sp,0x10; nop` | 0x13DB48    |
| func_0023CD00  | 0x23CD00  | `addiu $sp,$sp,0x20; nop` | 0x13DC80    |

Investigation (2026-09-03):

- Ghidra has NO function at any of these addresses and reports no xrefs to
  them. Each sits in an unanalyzed 4-byte gap between two real functions
  (e.g. FUN_0023ac90 ends `jr ra / addiu sp,+0x10` at 0x23ACAC, the next real
  function starts at 0x23ACB8).
- Nothing branches to them, so they are unreachable: likely leftover epilogue
  words from an earlier revision of the adjacent function, left in place by
  the original build (no relocation/xref evidence either way; boot ELF is
  stripped so no symbol oracle).
- A C function cannot compile to a bare `addiu $sp,...; nop` with no return,
  and GCC would emit `.p2align` padding that breaks the tight 4-byte layout.

Status: NOT decompilable as C/C++; treat as permanent assembly (or future
work would require raw asm blocks / linker tricks). Left in INCLUDE_ASM so
the build keeps matching. Do not pick these as targets.
